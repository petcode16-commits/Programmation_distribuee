#!/bin/bash

echo "[PROJET] Arrêt de tous les modules..."

# Tuer les processus C
killall central 2>/dev/null
killall thermometre 2>/dev/null
killall chauffage 2>/dev/null
killall console_control 2>/dev/null
killall console_cmd 2>/dev/null

# Tuer les processus Java (Air et RMIServer)
# On cherche les noms de classe spécifiques pour ne pas tuer d'autres applis Java
pkill -f "java Air"
pkill -f "java RMIServer"
pkill -f "java ConsoleRMI"

# Tuer le registre RMI s'il a été lancé séparément
pkill -f "rmiregistry"

echo "[PROJET] Système arrêté proprement."