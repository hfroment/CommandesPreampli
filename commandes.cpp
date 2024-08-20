#include <Wire.h>

#define SCL_PIN 3
#define SCL_PORT PORTD
#define SDA_PIN 2
#define SDA_PORT PORTD

#include <SoftWire.h>

#include "commandes.h"

const uint8_t Commandes::bpPin[Commandes::nombreDeBoutons] = {5, 6, A2, A3};
const ActionsServitudes::teCibleActionServitudes Commandes::bpAction[Commandes::nombreDeBoutons] = {
    ActionsServitudes::AlimAmpliTerrasse,
    ActionsServitudes::AlimAmpliVeranda,
    ActionsServitudes::AlimAux1,
    ActionsServitudes::AlimAmpliSalon
};

Commandes* Commandes::mInstance = nullptr;

Commandes::Commandes() :
    mActionsEmpilees(new CircularBuffer<Action>(mNombreActionsEmpilables)),
    mCommandesInternesCourantes(AucuneEntreeInterne),
    mCommandesExternesCourantes(AucuneEntreeExterne)
    , mI2c(0),
    mDebug(false)
{
    for (int i = 0; i < ActionsServitudes::NombreCibles; i++)
    {
        mDelaisExtinctionCourant[i] = 0;
        mDelaisExtinctionDemande[i] = 0;
    }
    mDelaisExtinctionDemande[ActionsServitudes::AlimAmpliSalon] = mDelaisExtinction;
}

void Commandes::gerer(bool topSeconde)
{
    // On lit les boutons
    for (int i = 0; i < nombreDeBoutons; i++)
    {
        if (mBounce[i].update() == 1)
        {
//            Serial.print(F("BP ");
//            Serial.print(i);
            if (mBounce[i].read() == 0)
            {
                // Falling edge
//                Serial.println(F(" falling edge ");
                traiterActionAppui(bpAction[i]);
            }
            else
            {
                // Raising edge
//                Serial.println(F(" raising edge ");
                traiterActionRelache(bpAction[i]);
            }
        }
    }
    // On gère les appuis longs
    traiterPowerOff();
    // On s'occupe des actions secondes
    if (topSeconde)
    {
        // extinctions différées
        for (int i = 0; i < ActionsServitudes::NombreCibles; i++)
        {
            if (mDelaisExtinctionCourant[i] > 0)
            {
                if (mDelaisExtinctionCourant[i] == 1)
                {
                    Serial.print(F("Exinction différée : "));
                    Serial.println(i, DEC);
                    traiterAction(static_cast<ActionsServitudes::teCibleActionServitudes>(i), ActionsServitudes::Off);
                }
                mDelaisExtinctionCourant[i]--;
            }
        }
        // Commandes liaison série
        processSerial();
        if (mDebug)
        {
            Serial.print(F("* "));
            Serial.print(mCommandesInternesCourantes, HEX);
            Serial.print(F(" "));
            Serial.print(mCommandesExternesCourantes, HEX);
            Serial.print(F(" "));
//            for (int i = 0; i < ActionsServitudes::NombreCibles; i++)
//            {
//                if (mDelaisExtinctionCourant[i] > 0)
//                {
//                    Serial.print(F("*D "));
//                    Serial.print(i);
//                    Serial.print(F(" "));
//                    Serial.println(mDelaisExtinctionCourant[i]);
//                }
//            }
        }
    }
    // on gére les actions empilées
    Action action;
    while (depilerAction(action))
    {
//        Serial.print(F("Dépiler action : ");
//        Serial.print(action.mCible, DEC);
//        Serial.print(F(", ");
//        Serial.println(action.mType, DEC);
        traiterAction(action.mCible, action.mType);
    }
}

void Commandes::init()
{
    mInstance = this;
    // Hardware i2c
    Wire.begin(DialogDefinition::servitudesI2cId);
    Wire.onReceive(Commandes::receiveEvent);
    Wire.onRequest(Commandes::sendAnswer);
    // Software i2c
    if (mI2c == 0)
    {
        mI2c = new SoftWire();
        mI2c->begin();
    }
    // BP
    for (int i = 0; i < nombreDeBoutons; i++)
    {
//        if (mBounce[i] == 0)
//        {
//            Bounce* bounce = new Bounce();
            pinMode(bpPin[i], INPUT_PULLUP);
//            mBounce[i] = new Bounce();
            mBounce[i].attach(bpPin[i]);
            mBounce[i].interval(5);
//            mBounce[i]->update();
//            Serial.print(F("BP ");
//            Serial.print(mBounce[i] == 0);
//            Serial.print(F("BP ");
//            Serial.print(bounce == 0);
//            bounce->update();
//        }
    }
    pinMode(powerOffPin, OUTPUT);
    digitalWrite(powerOffPin, LOW);

    // Lecture et report de la configuration
    mCommandesInternesCourantes = AucuneEntreeInterne; //Configuration::instance()->commandesInternesCourantes();
    envoyerCommandesInternes();
    mCommandesExternesCourantes = AucuneEntreeExterne; //Configuration::instance()->commandesExternesCourantes();
}


void Commandes::activerCommandeInterne(teEtatCommandeInternes commande)
{
    // logique inversée sur les cartes relais
    mCommandesInternesCourantes &= ~commande;
    envoyerCommandesInternes();
}

void Commandes::desactiverCommandeInterne(teEtatCommandeInternes commande)
{
    // logique inversée sur les cartes relais
    mCommandesInternesCourantes |= commande;
    envoyerCommandesInternes();
}

void Commandes::activerCommandeInterneLocale(teEtatCommandeInternes commande)
{
    mCommandesInternesCourantes |= commande;
    envoyerCommandesInternes();
}

void Commandes::desactiverCommandeInterneLocale(teEtatCommandeInternes commande)
{
    mCommandesInternesCourantes &= ~commande;
    envoyerCommandesInternes();
}

void Commandes::activerCommandeExterne(teEtatCommandeExternes commande)
{
    mCommandesExternesCourantes |= commande;
    envoyerCommandesExternes();
}

void Commandes::desactiverCommandeExterne(teEtatCommandeExternes commande)
{
    mCommandesExternesCourantes &= ~commande;
    envoyerCommandesExternes();
}

void Commandes::toggleCommandeInterne(teEtatCommandeInternes commande)
{
    // logique inversée sur les cartes relais
    if ((mCommandesInternesCourantes & commande) != 0)
    {
        activerCommandeInterne(commande);
    }
    else
    {
        desactiverCommandeInterne(commande);
    }
}

void Commandes::toggleCommandeInterneLocale(teEtatCommandeInternes commande)
{
    if ((mCommandesInternesCourantes & commande) == 0)
    {
        activerCommandeInterneLocale(commande);
    }
    else
    {
        desactiverCommandeInterneLocale(commande);
    }
}

void Commandes::toggleCommandeExterne(teEtatCommandeExternes commande)
{
    if ((mCommandesExternesCourantes & commande) == 0)
    {
        activerCommandeExterne(commande);
    }
    else
    {
        desactiverCommandeExterne(commande);
    }
}

void Commandes::envoyerCommandesInternes()
{
    mI2c->beginTransmission(mPcfCommandesInternes);
    mI2c->write(mCommandesInternesCourantes);
    mI2c->endTransmission();
}

void Commandes::envoyerCommandesExternes()
{
    mI2c->beginTransmission(mPcfCommandesExternes);
    mI2c->write(mCommandesExternesCourantes);
    mI2c->endTransmission();
    //Configuration::instance()->setCommandesExternesCourantes(mCommandesExternesCourantes);
}

void Commandes::traiterActionAppui(ActionsServitudes::teCibleActionServitudes cible)
{
    if (cible == ActionsServitudes::PowerOff)
    {
        //mDateDernierAppuiPowerOff = millis();
    }
    else
    {
        traiterAction(cible, ActionsServitudes::Toggle);
    }
}

void Commandes::traiterActionRelache(ActionsServitudes::teCibleActionServitudes cible)
{
    if (cible == ActionsServitudes::PowerOff)
    {
        //mDateDernierAppuiPowerOff = 0;
    }
}

void Commandes::traiterAction(ActionsServitudes::teCibleActionServitudes cible, ActionsServitudes::teTypeActionServitudes type)
{
//    Serial.print(F("Action : ");
//    Serial.print(cible, DEC);
//    Serial.print(F(", ");
//    Serial.println(type, DEC);
    teEtatCommandeExternes commandeExterne = AucuneEntreeExterne;
    teEtatCommandeInternes commandeInterne = AucuneEntreeInterne;

    switch(cible)
    {
    case ActionsServitudes::PowerOff:
        digitalWrite(powerOffPin, HIGH);
        break;
    case ActionsServitudes::PowerOffReleased:
        digitalWrite(powerOffPin, LOW);
        break;
    case ActionsServitudes::Out1:
        commandeInterne = EtatOut1;
        break;
    case ActionsServitudes::Out2:
        commandeInterne = EtatOut2;
        break;
    case ActionsServitudes::Out3:
        commandeInterne = EtatOut3;
        break;
    case ActionsServitudes::Out4:
        commandeInterne = EtatOut4;
        break;
    case ActionsServitudes::Out5:
        commandeInterne = EtatOut5;
        break;
    case ActionsServitudes::Out6:
        commandeInterne = EtatOut6;
        break;
    case ActionsServitudes::Out7:
        commandeInterne = EtatOut7;
        break;
    case ActionsServitudes::Out8:
        commandeInterne = EtatOut8;
        break;
    case ActionsServitudes::AlimKodi:
        commandeExterne = EtatAlimKodi;
        break;
    case ActionsServitudes::AlimAmpliTerrasse:
        commandeExterne = EtatAlimAmpliTerrasse;
        break;
    case ActionsServitudes::AlimAmpliVeranda:
        commandeExterne = EtatAlimAmpliVeranda;
        break;
    case ActionsServitudes::AlimAux2:
        commandeExterne = EtatAlimAux2;
        break;
    case ActionsServitudes::AlimAux1:
        commandeExterne = EtatAlimAux1;
        break;
    case ActionsServitudes::AlimAmpliSalon:
        commandeExterne = EtatAlimAmpliSalon;
        break;
    case ActionsServitudes::Aux12V_1:
        commandeExterne = EtatAux12V_1;
        break;
    case ActionsServitudes::Aux12V_2:
        commandeExterne = EtatAux12V_2;
        break;
    }

    switch(type)
    {
    case ActionsServitudes::Off:
        // Pour l'extinction, on peut avoir du délai
        if (mDelaisExtinctionDemande[cible] > 0)
        {
            if (mDelaisExtinctionCourant[cible] == 0)
            {
                // début de cycle
                mDelaisExtinctionCourant[cible] = mDelaisExtinctionDemande[cible];
            }
            else if (mDelaisExtinctionCourant[cible] == 1)
            {
                // extinction
                if (commandeInterne != AucuneEntreeInterne)
                {
                    desactiverCommandeInterne(commandeInterne);
                }
                if (commandeExterne != AucuneEntreeExterne)
                {
                    desactiverCommandeExterne(commandeExterne);
                }
            }
        }
        else
        {
            if (commandeInterne != AucuneEntreeInterne)
            {
                desactiverCommandeInterne(commandeInterne);
            }
            if (commandeExterne != AucuneEntreeExterne)
            {
                desactiverCommandeExterne(commandeExterne);
            }
        }
        break;
    case ActionsServitudes::On:
        mDelaisExtinctionCourant[cible] = 0;
        if (commandeInterne != AucuneEntreeInterne)
        {
            activerCommandeInterne(commandeInterne);
        }
        if (commandeExterne != AucuneEntreeExterne)
        {
            activerCommandeExterne(commandeExterne);
        }
        break;
    case ActionsServitudes::Toggle:
        if (commandeInterne != AucuneEntreeInterne)
        {
            toggleCommandeInterne(commandeInterne);
        }
        if (commandeExterne != AucuneEntreeExterne)
        {
            toggleCommandeExterne(commandeExterne);
        }
        break;
    }
}

void Commandes::empilerAction(ActionsServitudes::teCibleActionServitudes cible, ActionsServitudes::teTypeActionServitudes type)
{
    mActionsEmpilees->put(Action(cible, type));
}

bool Commandes::depilerAction(Action &action)
{
    bool retour = false;

    if (!mActionsEmpilees->empty())
    {
        Action actionDepilee = mActionsEmpilees->get();
        action.mCible = actionDepilee.mCible;
        action.mType = actionDepilee.mType;
        retour = true;
    }

    return retour;
}

void Commandes::traiterPowerOff()
{
//    if ((mDateDernierAppuiPowerOff > 0) && (millis() - mDateDernierAppuiPowerOff > mDurreAppuiLong))
//    {
//        if (mStandbyMode)
//        {
//            Serial.println(F("PowerOffReleased"));
//            traiterAction(ActionsServitudes::PowerOffReleased, ActionsServitudes::Toggle);
//        }
//        else
//        {
//            Serial.println(F("PowerOff"));
//            traiterAction(ActionsServitudes::PowerOff, ActionsServitudes::Toggle);
//        }
//        mStandbyMode = !mStandbyMode;
//        mDateDernierAppuiPowerOff = 0;
//    }

}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void Commandes::receiveEvent(int howMany)
{
    if (mInstance != nullptr)
    {
//        Serial.print(F("receiveEvent : ");
//        Serial.println(howMany);         // print the integer
        ActionsServitudes::teCibleActionServitudes cible = ActionsServitudes::NombreCibles;
        if (Wire.available())
        {
            uint8_t c = Wire.read();
//            Serial.println(c);         // print the integer
            cible = static_cast<ActionsServitudes::teCibleActionServitudes>(c);
        }
        if (Wire.available())
        {
            uint8_t c = Wire.read();
//            Serial.println(c);         // print the integer
            ActionsServitudes::teTypeActionServitudes action = static_cast<ActionsServitudes::teTypeActionServitudes>(c);
            mInstance->empilerAction(cible, action);
        }
        while (Wire.available())
        {
            Wire.read();
        }
    }
    else
    {
//        Serial.println(F("receiveEvent : mInstance is null");
    }
}
/*
 * Send a byte to Master
 */
void Commandes::sendAnswer()
{
    if (mInstance != nullptr)
    {
        Wire.write(mInstance->mCommandesExternesCourantes);
        Wire.write((mInstance->mCommandesInternesCourantes & ~EtatOut7) & ~EtatOut8);
    }
    else
    {
//        Serial.println(F("sendAnswer : mInstance is null");
    }
}

void Commandes::processSerial()
{
    // if there's any serial available, read it:
    while (Serial.available() > 0) {

        // look for the next valid integer in the incoming serial stream:
        uint8_t lu = Serial.parseInt();
        ActionsServitudes::teCibleActionServitudes cible = static_cast<ActionsServitudes::teCibleActionServitudes>(lu);
        // do it again:
        lu = Serial.parseInt();
        ActionsServitudes::teTypeActionServitudes action = static_cast<ActionsServitudes::teTypeActionServitudes>(lu);

        // look for the newline. That's the end of your sentence:
        if (Serial.read() == '\n') {
            if (lu == 99)
            {
                mDebug = true;
            }
            else
            {
                empilerAction(cible, action);
            }
        }
    }
}
