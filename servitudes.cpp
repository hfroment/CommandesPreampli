#include <TimerOne.h>

#include "commandes.h"

#include "servitudes.h"

Servitudes *Servitudes::instance = NULL;
const String Servitudes::mVersionString = "0.0.1";

Servitudes::Servitudes() :
    mCompteurIt(0),
    mCompteurItPrecedent(255),
    mCommandes(* new Commandes())
{
    instance = this;

    pinMode(ledPin, OUTPUT);
    // On met en place l'IT seconde
    Timer1.initialize(1000000);
    Timer1.attachInterrupt(Servitudes::instance->cadencer1s);
}

void Servitudes::init()
{
    //Configuration::instance()->charger();
    mCommandes.init();
}

bool Servitudes::gerer()
{
    bool topSeconde = false;
    if (mCompteurIt != mCompteurItPrecedent)
    {
        digitalWrite(ledPin, 1);
        mCompteurItPrecedent = mCompteurIt;
//        if (mCommandes.standbyMode())
        {
            delay(5);
            digitalWrite(ledPin, 0);
        }
        topSeconde = true;
    }

    mCommandes.gerer(topSeconde);
    //Configuration::instance()->gerer();

    return true;
}

void Servitudes::cadencer1s()
{
    instance->mCompteurIt++;
}

