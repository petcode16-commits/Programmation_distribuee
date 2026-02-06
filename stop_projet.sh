#!/bin/bash
tmux kill-session -t domotique 2>/dev/null
pkill -f "java Air"
pkill -f "java RMIServer"
pkill -f "java ConsoleRMI"
pkill -x central thermometre chauffage console_control console_cmd
echo "✅ Système arrêté."