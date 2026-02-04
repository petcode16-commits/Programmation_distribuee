===========================================================================
          PROJET : SYSTÈME DOMOTIQUE DISTRIBUÉ (C / JAVA RMI)
===========================================================================

Ce projet implémente un système de gestion thermique intelligent. Il simule 
la température de plusieurs pièces, permet leur monitoring en temps réel 
et leur pilotage via différentes interfaces (Consoles C et Console Java RMI).

---------------------------------------------------------------------------
1. ARCHITECTURE DU SYSTÈME
---------------------------------------------------------------------------

Le projet repose sur une architecture hybride distribuée :

* CŒUR (C) : 
    Serveur 'central' qui multiplexe les communications TCP et sert de 
    pont UDP pour le monde Java.

* SIMULATION PHYSIQUE (Java) : 
    Programme 'Air' qui gère l'inertie thermique d'une pièce.

* MODULES TERMINAUX (C) :
    - thermometre : Capte la température via Multicast et informe le 
      Central via TCP.
    - chauffage   : Reçoit les ordres du Central (TCP) et agit sur la 
      simulation (Multicast).

* INTERFACES DE CONTRÔLE :
    - C : 'console_control' (monitoring) et 'console_cmd' (pilotage).
    - Java RMI : 'RMIServer' (serveur d'objets) et 'ConsoleRMI' (pont).

---------------------------------------------------------------------------
2. PRÉ-REQUIS
---------------------------------------------------------------------------

- OS      : Linux (Ubuntu recommandé)
- Outils  : tmux installé (sudo apt install tmux)
- Langages: GCC (C) et JDK (Java)

---------------------------------------------------------------------------
3. COMPILATION ET LANCEMENT
---------------------------------------------------------------------------

A. Compilation automatique :
   Tapez la commande suivante dans le dossier racine :
   $ make clean && make && javac *.java

B. Lancement avec TMUX (Recommandé) :
   Le script 'run_tmux.sh' déploie l'infrastructure (11 panneaux / 5 fenêtres) :
   $ chmod +x run_tmux.sh
   $ ./run_tmux.sh

---------------------------------------------------------------------------
4. ORGANISATION DES FENÊTRES TMUX
---------------------------------------------------------------------------

Fenêtre 0: [Serveurs]   -> central, RMIServer, ConsoleRMI
Fenêtre 1: [Salon]      -> Air, thermo, chauffage (Multicast Port 5000)
Fenêtre 2: [Cuisine]    -> Air, thermo, chauffage (Multicast Port 5001)
Fenêtre 3: [Chambre]    -> Air, thermo, chauffage (Multicast Port 5002)
Fenêtre 4: [Consoles_C] -> console_control, console_cmd

---------------------------------------------------------------------------
5. COMMANDES UTILES DANS TMUX
---------------------------------------------------------------------------

- Changer de fenêtre      : Ctrl+B puis le chiffre (0 à 4)
- Zoomer/Dézoomer         : Ctrl+B puis z
- Naviguer entre panneaux : Ctrl+B puis les flèches directionnelles
- Quitter la session      : Ctrl+B puis taper ":kill-session" + Entrée

---------------------------------------------------------------------------
6. PROTOCOLES ET PORTS
---------------------------------------------------------------------------

* TCP (8080)         : Enregistrement modules, ordres SET et UPDATE.
* UDP Multicast (500x): Flux binaire Air <-> Modules.
* UDP Unicast (9090)  : Communication Pont Java <-> Central C.
* Java RMI (1099)     : Invocation de méthodes distantes.

---------------------------------------------------------------------------
7. DÉPANNAGE (TROUBLESHOOTING)
---------------------------------------------------------------------------

- Erreur "Address already in use" : 
  Tuez les processus avec : fuser -k 8080/tcp ou pkill -9 central

- Problème RMI : 
  Assurez-vous qu'aucune instance de 'rmiregistry' ne tourne déjà sur le 
  port 1099 avant de lancer RMIServer.

===========================================================================
Développé dans le cadre du module Réseau et Systèmes Distribués.
===========================================================================