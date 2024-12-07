#include <Wire.h>

#include "commandes.h"

const uint8_t Commandes::bpPin[Commandes::nombreDeBoutons] = {10, 9, 8, 7, A0};
const ActionsServitudes::teCibleActionServitudes Commandes::bpAction[Commandes::nombreDeBoutons] = {
    ActionsServitudes::AlimAmpliSalon,
    ActionsServitudes::AlimAmpliTerrasse,
    ActionsServitudes::AlimAmpliVeranda,
    ActionsServitudes::AlimAux1,
    ActionsServitudes::AlimAux2
};
const uint8_t Commandes::mLedPin[ActionsServitudes::NombreCibles] = {0, 13, A2, A3, 2, 3};
const uint8_t Commandes::mOutPin[ActionsServitudes::NombreCibles] = {12, 11, 4, 5, 6, A1};

Commandes* Commandes::mInstance = nullptr;

Commandes::Commandes() :
    mActionsEmpilees(new CircularBuffer<Action>(mNombreActionsEmpilables)),
    mDebug(false),
    mStatus(0)
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
    //    // On gère les appuis longs
    //    traiterPowerOff();
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

    for (int i = 0; i < ActionsServitudes::NombreCibles; i++)
    {
        if (mLedPin[i] != 0)
        {
            pinMode(mLedPin[i], OUTPUT);
            digitalWrite(mLedPin[i],LOW);
            digitalWrite(mLedPin[i],HIGH);
            delay(200);
            digitalWrite(mLedPin[i],LOW);
        }
        if (mOutPin[i] != 0)
        {
            pinMode(mOutPin[i], OUTPUT);
            digitalWrite(mOutPin[i],LOW);
        }
    }

    traiterAction(ActionsServitudes::AlimAmpliSalon, ActionsServitudes::On);
    traiterAction(ActionsServitudes::AlimKodi, ActionsServitudes::On);
}

void Commandes::traiterActionAppui(ActionsServitudes::teCibleActionServitudes cible)
{
    //    if (cible == ActionsServitudes::PowerOff)
    //    {
    //        //mDateDernierAppuiPowerOff = millis();
    //    }
    //    else
    {
        traiterAction(cible, ActionsServitudes::Toggle);
    }
}

void Commandes::traiterActionRelache(ActionsServitudes::teCibleActionServitudes cible)
{
    //    if (cible == ActionsServitudes::PowerOff)
    //    {
    //        //mDateDernierAppuiPowerOff = 0;
    //    }
}

void Commandes::traiterAction(ActionsServitudes::teCibleActionServitudes cible, ActionsServitudes::teTypeActionServitudes type)
{
    //    Serial.print(F("Action : ");
    //    Serial.print(cible, DEC);
    //    Serial.print(F(", ");
    //    Serial.println(type, DEC);

    if (mOutPin[cible] != 0)
    {
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
                    digitalWrite(mOutPin[cible], LOW);
                }
            }
            else
            {
                // extinction
                digitalWrite(mOutPin[cible], LOW);
            }
            break;
        case ActionsServitudes::On:
            mDelaisExtinctionCourant[cible] = 0;
            // allumage
            digitalWrite(mOutPin[cible], HIGH);
            break;
        case ActionsServitudes::Toggle:
            digitalWrite(mOutPin[cible], !digitalRead(mOutPin[cible]));
            break;
        }
        if (mLedPin[cible] != 0)
        {
            digitalWrite(mLedPin[cible], digitalRead(mOutPin[cible]));
        }
    }
    processStatus();
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

//void Commandes::traiterPowerOff()
//{
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

//}

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
        Wire.write(mInstance->mStatus);
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

void Commandes::processStatus()
{
    mStatus = 0;
    for (int i = 0; i < ActionsServitudes::NombreCibles; i++)
    {
        uint8_t pin = mOutPin[i];
        if (digitalRead(pin) == HIGH)
        {
            mStatus |= 1 << i;
        }
    }
}
