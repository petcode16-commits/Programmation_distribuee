#!/bin/bash

echo "[PROJET] Arrêt de tous les modules et fermeture des fenêtres..."

# 1. On ferme la session tmux (cela tue tout ce qui tourne dedans proprement)
tmux kill-session -t domotique 2>/dev/null

# 2. Sécurité : On tue les processus qui pourraient avoir survécu 
# (au cas où ils auraient été lancés hors tmux)
killall central thermometre chauffage console_control console_cmd 2>/dev/null

# 3. Tuer les processus Java spécifiques
pkill -f "java Air"
pkill -f "java RMIServer"
pkill -f "java ConsoleRMI"

# 4. Nettoyage des fichiers temporaires (si nécessaire)
# rm -f /tmp/domotique_pipe (exemple si tu utilisais des pipes)

echo "[PROJET] Tout le système a été arrêté avec succès."