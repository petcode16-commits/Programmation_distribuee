#!/bin/bash

echo "--- Arrêt complet du système ---"

# 1. Fermer la session Tmux
tmux kill-session -t domotique 2>/dev/null

# 2. Tuer proprement les processus Java par nom de classe
# On utilise SIGTERM (par défaut) puis SIGKILL si ça résiste
pkill -f "java Air"
pkill -f "java RMIServer"
pkill -f "java ConsoleRMI"

# 3. Tuer les processus C
pkill -x central
pkill -x thermometre
pkill -x chauffage
pkill -x console_control
pkill -x console_cmd

# 4. Petit nettoyage des fichiers temporaires (optionnel)
# rm -f *.class 

echo "✅ Tout est arrêté. Les ports sont libres."