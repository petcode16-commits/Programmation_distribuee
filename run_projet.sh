#!/bin/bash

# Configuration
PORT_TCP=8080
IP_MCAST="224.0.0.1"
PORT_MCAST=9000
SESSION="domotique"
HOST="127.0.0.1" # On utilise l'IP fixe pour éviter les problèmes de résolution

echo "--- Compilation du projet ---"
make clean && make
if [ $? -ne 0 ]; then echo "Erreur de compilation !"; exit 1; fi

# Nettoyage Tmux
tmux kill-session -t $SESSION 2>/dev/null
tmux new-session -d -s $SESSION -n "Serveurs"

# Fenêtre 0 : Central et RMI
tmux send-keys -t $SESSION:0 "./central $PORT_TCP" C-m
tmux split-window -h -t $SESSION:0
tmux send-keys -t $SESSION:0 "java RMIServer" C-m

# Fenêtre 1 : Salon
tmux new-window -t $SESSION -n "Salon"
tmux send-keys -t $SESSION:1 "java Air $IP_MCAST $PORT_MCAST Salon" C-m
tmux split-window -v -t $SESSION:1
tmux send-keys -t $SESSION:1 "./thermometre $IP_MCAST $PORT_MCAST Salon $HOST $PORT_TCP" C-m
tmux split-window -h -t $SESSION:1
tmux send-keys -t $SESSION:1 "./chauffage $IP_MCAST $PORT_MCAST Salon $HOST $PORT_TCP" C-m

# Fenêtre 2 : Chambre
tmux new-window -t $SESSION -n "Chambre"
tmux send-keys -t $SESSION:2 "java Air $IP_MCAST $PORT_MCAST Chambre" C-m
tmux split-window -v -t $SESSION:2
tmux send-keys -t $SESSION:2 "./thermometre $IP_MCAST $PORT_MCAST Chambre $HOST $PORT_TCP" C-m
tmux split-window -h -t $SESSION:2
tmux send-keys -t $SESSION:2 "./chauffage $IP_MCAST $PORT_MCAST Chambre $HOST $PORT_TCP" C-m

# Fenêtre 3 : Cuisine
tmux new-window -t $SESSION -n "Cuisine"
tmux send-keys -t $SESSION:3 "java Air $IP_MCAST $PORT_MCAST Cuisine" C-m
tmux split-window -v -t $SESSION:3
tmux send-keys -t $SESSION:3 "./thermometre $IP_MCAST $PORT_MCAST Cuisine $HOST $PORT_TCP" C-m
tmux split-window -h -t $SESSION:3
tmux send-keys -t $SESSION:3 "./chauffage $IP_MCAST $PORT_MCAST Cuisine $HOST $PORT_TCP" C-m

# Fenêtre 4 : Consoles
tmux new-window -t $SESSION -n "Consoles"
tmux send-keys -t $SESSION:4 "./console_control $HOST $PORT_TCP" C-m
tmux split-window -v -t $SESSION:4
tmux send-keys -t $SESSION:4 "./console_cmd $HOST $PORT_TCP" C-m
tmux split-window -h -t $SESSION:4
tmux send-keys -t $SESSION:4 "sleep 2; java ConsoleRMI" C-m

tmux select-window -t $SESSION:4
tmux attach-session -t $SESSION