#ifndef SERVITUDES_H
#define SERVITUDES_H

class Commandes;

class Servitudes
{
public:
    Servitudes();

    void init();
    // Fonction périodique
    bool gerer();

    static void cadencer1s();

private:
    static Servitudes *instance;

    uint8_t mCompteurIt;
    uint8_t mCompteurItPrecedent;

    static const String mVersionString;

    static const uint8_t ledPin = 13;

    Commandes& mCommandes;
};

#endif // SERVITUDES_H
