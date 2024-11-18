#ifndef COMMANDES_H
#define COMMANDES_H

#include <Arduino.h>

//#include "options.h"

#include "circularbuffer.h"

#include "actions.h"

#include <Bounce2.h>

class SoftWire;

class Commandes
{
public:
    Commandes();
    void gerer(bool topSeconde);
    void init();

private:
    class Action
    {
    public:
        Action(ActionsServitudes::teCibleActionServitudes cible = ActionsServitudes::NombreCibles,
               ActionsServitudes::teTypeActionServitudes type = ActionsServitudes::Off) :
            mCible(cible),
            mType(type)
        {}
        ActionsServitudes::teCibleActionServitudes mCible;
        ActionsServitudes::teTypeActionServitudes mType;
    };

    /// Le nombre d'actions empliables
    static const size_t mNombreActionsEmpilables = 16;
    /// La pile d'actions
    CircularBuffer<Action>* mActionsEmpilees;


    static const uint8_t nombreDeBoutons = 5;
    static const uint8_t bpPin[nombreDeBoutons];
    static const ActionsServitudes::teCibleActionServitudes bpAction[nombreDeBoutons];
    static const uint8_t mLedPin[nombreDeBoutons];
    static const uint8_t mOutPin[ActionsServitudes::NombreCibles];

    void traiterActionAppui(ActionsServitudes::teCibleActionServitudes cible);
    void traiterActionRelache(ActionsServitudes::teCibleActionServitudes cible);
    void traiterAction(ActionsServitudes::teCibleActionServitudes cible, ActionsServitudes::teTypeActionServitudes type);
    void empilerAction(ActionsServitudes::teCibleActionServitudes cible, ActionsServitudes::teTypeActionServitudes type);
    bool depilerAction(Action& action);

//    void traiterPowerOff();

    Bounce mBounce[nombreDeBoutons];

    /// Les délais d'extinction
    uint8_t mDelaisExtinctionCourant[ActionsServitudes::NombreCibles];
    uint8_t mDelaisExtinctionDemande[ActionsServitudes::NombreCibles];
    static const uint8_t mDelaisExtinction = 60; // s

    static void receiveEvent(int howMany);
    static void sendAnswer();

    void processSerial();

    /// L'instance
    static Commandes* mInstance;

    /// le mode debug
    bool mDebug;

    uint8_t mStatus;

    void processStatus();
};

#endif // COMMANDES_H
