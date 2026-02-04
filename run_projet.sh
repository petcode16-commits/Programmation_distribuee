#!/bin/bash

# Configuration
SESSION="domotique"
CENTRAL_HOST="localhost"
CENTRAL_PORT=8080
UDP_BRIDGE_PORT=9090

# --- ÉTAPE 0 : COMPILATION ---
echo "[PROJET] Compilation en cours..."
make clean && make
javac *.java

# Détruire une ancienne session si elle existe
tmux kill-session -t $SESSION 2>/dev/null

# --- ÉTAPE 1 : SERVEURS (Fenêtre 0) ---
# Panneau 1 : Central C | Panneau 2 : RMIServer Java | Panneau 3 : ConsoleRMI (Pont)
tmux new-session -d -s $SESSION -n "Serveurs" "./central $CENTRAL_PORT"
tmux split-window -h -t $SESSION "java RMIServer; exec bash"
tmux split-window -v -t $SESSION "java ConsoleRMI ; exec bash"

# --- ÉTAPE 2 : SALON - Port 5000 (Fenêtre 1) ---
tmux new-window -t $SESSION -n "Salon" "java Air 224.0.0.1 5000 Salon; exec bash"
tmux split-window -h -t $SESSION "./thermometre 224.0.0.1 5000 Salon $CENTRAL_HOST $CENTRAL_PORT; exec bash"
tmux split-window -v -t $SESSION "./chauffage 224.0.0.1 5000 Salon $CENTRAL_HOST $CENTRAL_PORT; exec bash"

# --- ÉTAPE 3 : CUISINE - Port 5001 (Fenêtre 2) ---
tmux new-window -t $SESSION -n "Cuisine" "java Air 224.0.0.1 5001 Cuisine; exec bash"
tmux split-window -h -t $SESSION "./thermometre 224.0.0.1 5001 Cuisine $CENTRAL_HOST $CENTRAL_PORT; exec bash"
tmux split-window -v -t $SESSION "./chauffage 224.0.0.1 5001 Cuisine $CENTRAL_HOST $CENTRAL_PORT; exec bash"

# --- ÉTAPE 4 : CHAMBRE - Port 5002 (Fenêtre 3) ---
tmux new-window -t $SESSION -n "Chambre" "java Air 224.0.0.1 5002 Chambre; exec bash"
tmux split-window -h -t $SESSION "./thermometre 224.0.0.1 5002 Chambre $CENTRAL_HOST $CENTRAL_PORT; exec bash"
tmux split-window -v -t $SESSION "./chauffage 224.0.0.1 5002 Chambre $CENTRAL_HOST $CENTRAL_PORT; exec bash"

# --- ÉTAPE 5 : CONSOLES DE TEST C (Fenêtre 4) ---
tmux new-window -t $SESSION -n "Consoles_C" "./console_control $CENTRAL_HOST $CENTRAL_PORT; exec bash"
tmux split-window -v -t $SESSION "./console_cmd $CENTRAL_HOST $CENTRAL_PORT; exec bash"

# Retour à la fenêtre des serveurs et affichage
tmux select-window -t $SESSION:0
tmux attach-session -t $SESSION