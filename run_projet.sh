#!/bin/bash

# Configuration
PORT_TCP=8080
IP_MCAST="224.0.0.1"
PORT_MCAST=9000
SESSION="domotique"

echo "--- Compilation ---"
make clean && make
if [ $? -ne 0 ]; then echo "Erreur de compilation !"; exit 1; fi

# Nettoyage avant lancement
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
tmux send-keys -t $SESSION:1 "./thermometre $IP_MCAST $PORT_MCAST Salon localhost $PORT_TCP" C-m
tmux split-window -h -t $SESSION:1
tmux send-keys -t $SESSION:1 "./chauffage $IP_MCAST $PORT_MCAST Salon localhost $PORT_TCP" C-m

# Fenêtre 2 : Chambre
tmux new-window -t $SESSION -n "Chambre"
tmux send-keys -t $SESSION:2 "java Air $IP_MCAST $PORT_MCAST Chambre" C-m
tmux split-window -v -t $SESSION:2
tmux send-keys -t $SESSION:2 "./thermometre $IP_MCAST $PORT_MCAST Chambre localhost $PORT_TCP" C-m
tmux split-window -h -t $SESSION:2
tmux send-keys -t $SESSION:2 "./chauffage $IP_MCAST $PORT_MCAST Chambre localhost $PORT_TCP" C-m

# Fenêtre 3 : Cuisine
tmux new-window -t $SESSION -n "Cuisine"
tmux send-keys -t $SESSION:3 "java Air $IP_MCAST $PORT_MCAST Cuisine" C-m
tmux split-window -v -t $SESSION:3
tmux send-keys -t $SESSION:3 "./thermometre $IP_MCAST $PORT_MCAST Cuisine localhost $PORT_TCP" C-m
tmux split-window -h -t $SESSION:3
tmux send-keys -t $SESSION:3 "./chauffage $IP_MCAST $PORT_MCAST Cuisine localhost $PORT_TCP" C-m

# Fenêtre 4 : Consoles (Monitoring + Commande)
tmux new-window -t $SESSION -n "Consoles"
# En haut : La console de monitoring (affichage seul)
tmux send-keys -t $SESSION:4 "./console_control localhost $PORT_TCP" C-m
# En bas à gauche : La console de commande (envoi d'ordres)
tmux split-window -v -t $SESSION:4
tmux send-keys -t $SESSION:4 "./console_cmd localhost $PORT_TCP" C-m
# En bas à droite : La console RMI (Java)
tmux split-window -h -t $SESSION:4
tmux send-keys -t $SESSION:4 "sleep 2; java ConsoleRMI" C-m

# Retour à la vue des consoles pour commencer le test
tmux select-window -t $SESSION:4
tmux attach-session -t $SESSION