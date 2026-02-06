========================================================================
       SYSTÈME DE DOMOTIQUE DISTRIBUÉ (C / JAVA RMI / MULTICAST)
========================================================================

1. PRÉSENTATION DU PROJET
-------------------------
Ce projet simule un système de contrôle de chauffage intelligent pour 
une maison. Il repose sur une architecture distribuée mélangeant :
- Le langage C pour la performance et la gestion réseau bas niveau.
- Java (RMI) pour la gestion de l'interface de contrôle distante.
- L'UDP Multicast pour la simulation physique (température de l'air).
- Le TCP pour la communication fiable entre les modules et le Central.

2. ARCHITECTURE TECHNIQUE
-------------------------
- SERVEUR CENTRAL (C) : Le pivot du système. Il gère les connexions TCP
  des modules, valide les commandes, assure la résilience en cas de 
  panne et sert de pont UDP avec le monde Java.
- SIMULATION AIR (Java) : Calcule l'évolution thermique d'une pièce 
  en fonction de la puissance de chauffe reçue par Multicast.
- THERMOMÈTRE (C) : Capte la température de la simulation (Multicast)
  et la transmet au Central (TCP).
- CHAUFFAGE (C) : Agit sur la pièce. Supporte deux modes :
    * MANUEL : Puissance fixée par l'utilisateur (0 à 5).
    * AUTO : Régulation intelligente pour atteindre une température cible.
- CONSOLES (C & Java RMI) : Permettent de monitorer les pièces et 
  d'envoyer des ordres de commande.

3. FONCTIONNALITÉS AVANCÉES
---------------------------
- GESTION D'ERREURS : 
    * Validation stricte des saisies utilisateur (Puissance 0-5).
    * Vérification de l'existence de la pièce avant envoi de commande.
    * Protection contre les injections de texte à la place de nombres.
- RÉSILIENCE ET ROBUSTESSE :
    * Le Central ne crash pas si un module se déconnecte brusquement.
    * Utilisation de SIGPIPE (C) et Try-with-resources (Java).
    * Les autres pièces continuent de fonctionner si l'une d'elles tombe.
- MODE THERMOSTAT (AUTO) :
    * Implémentation d'une boucle de régulation dans le module chauffage.

4. INSTALLATION ET COMPILATION
------------------------------
Pré-requis : Un environnement Linux avec GCC, Java JDK et Tmux installé.

Compilation automatique via le Makefile :
   $ make clean
   $ make

Cela génère tous les exécutables C et compile les classes Java (.class).

5. UTILISATION
--------------
Pour lancer l'intégralité du système (3 pièces + serveurs + consoles) :
   $ chmod +x run_projet.sh
   $ ./run_projet.sh

Le script utilise TMUX pour organiser les fenêtres :
- Fenêtre 0 : Serveurs (Central, RMIServer, ConsoleRMI)
- Fenêtres 1-3 : Pièces (Salon, Cuisine, Chambre)
- Fenêtre 4 : Consoles de test C

Pour arrêter proprement tout le système :
   $ ./stop_projet.sh

6. PROTOCOLE DE COMMUNICATION
-----------------------------
- TCP : Commandes SET, AUTO et notifications UPDATE, STAT.
- UDP UNICAST (Port 9090/9091) : Dialogue entre le Central et Java RMI.
- UDP MULTICAST : Flux de données physiques (Température/Puissance).

7. DÉPANNAGE
------------
- Erreur "Port already in use" : Lancez ./stop_projet.sh pour libérer
  les ports réseaux restés ouverts.
- Erreur RMI : Vérifiez que le RMIServer est bien lancé avant la 
  ConsoleRMI. Le serveur crée automatiquement son propre registre.

========================================================================
                     RÉALISÉ DANS LE CADRE DU PROJET
========================================================================