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

    typedef enum
    {
        AucuneEntreeInterne = 0,
        EtatOut1 = 0x01,
        EtatOut2 = 0x02,
        EtatOut3 = 0x04,
        EtatOut4 = 0x08,
        EtatOut5 = 0x10,
        EtatOut6 = 0x20,
        EtatOut7 = 0x40,
        EtatOut8 = 0x80
    }
    teEtatCommandeInternes;

    typedef enum
    {
        AucuneEntreeExterne = 0,
        EtatAlimKodi = 0x01,
        EtatAlimAmpliTerrasse = 0x02,
        EtatAlimAmpliVeranda = 0x04,
        EtatAlimAux2 = 0x08,
        EtatAlimAux1 = 0x10,
        EtatAlimAmpliSalon = 0x20,
        EtatAux12V_1 = 0x40,
        EtatAux12V_2 = 0x80
    }
    teEtatCommandeExternes;

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

    /// Commandes interne
    uint8_t mCommandesInternesCourantes;
    /// Commandes externes
    uint8_t mCommandesExternesCourantes;

    static const uint8_t nombreDeBoutons = 4;
    static const uint8_t bpPin[nombreDeBoutons];
    static const ActionsServitudes::teCibleActionServitudes bpAction[nombreDeBoutons];

    void activerCommandeInterne(teEtatCommandeInternes commande);
    void activerCommandeInterneLocale(teEtatCommandeInternes commande);
    void desactiverCommandeInterne(teEtatCommandeInternes commande);
    void desactiverCommandeInterneLocale(teEtatCommandeInternes commande);
    void activerCommandeExterne(teEtatCommandeExternes commande);
    void desactiverCommandeExterne(teEtatCommandeExternes commande);
    void toggleCommandeInterne(teEtatCommandeInternes commande);
    void toggleCommandeInterneLocale(teEtatCommandeInternes commande);
    void toggleCommandeExterne(teEtatCommandeExternes commande);

    void envoyerCommandesInternes();
    void envoyerCommandesExternes();

    void traiterActionAppui(ActionsServitudes::teCibleActionServitudes cible);
    void traiterActionRelache(ActionsServitudes::teCibleActionServitudes cible);
    void traiterAction(ActionsServitudes::teCibleActionServitudes cible, ActionsServitudes::teTypeActionServitudes type);
    void empilerAction(ActionsServitudes::teCibleActionServitudes cible, ActionsServitudes::teTypeActionServitudes type);
    bool depilerAction(Action& action);

    void traiterPowerOff();

    Bounce mBounce[nombreDeBoutons];

    static const uint8_t powerOffPin = A0;

    SoftWire* mI2c;
    static const uint8_t mPcfCommandesInternes = 0x20;
    static const uint8_t mPcfCommandesExternes = 0x21;

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
};

#endif // COMMANDES_H
