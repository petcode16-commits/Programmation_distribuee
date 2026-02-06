# --- Configuration ---
CC = gcc
CFLAGS = -Wall -Wextra -g -D_GNU_SOURCE
LDFLAGS = 

# Liste des exécutables C
EXEC = central thermometre chauffage console_control console_cmd

# --- Règles principales ---
all: $(EXEC) java_modules

# Compilation de l'objet commun pour le protocole binaire
msg_temperature.o: msg_temperature.c msg_temperature.h
	$(CC) $(CFLAGS) -c msg_temperature.c -o msg_temperature.o

# Compilation des exécutables individuels
central: central.c
	$(CC) $(CFLAGS) central.c -o central

# Attention : thermometre et chauffage ont besoin de msg_temperature.o
thermometre: thermometre.c msg_temperature.o
	$(CC) $(CFLAGS) thermometre.c msg_temperature.o -o thermometre

chauffage: chauffage.c msg_temperature.o
	$(CC) $(CFLAGS) chauffage.c msg_temperature.o -o chauffage

console_control: console_control.c
	$(CC) $(CFLAGS) console_control.c -o console_control

console_cmd: console_cmd.c
	$(CC) $(CFLAGS) console_cmd.c -o console_cmd

# --- Modules Java ---
# Compile tous les .java présents dans le dossier
java_modules:
	javac *.java

# --- Nettoyage ---
clean:
	rm -f $(EXEC) *.o *.class
	@echo "Nettoyage terminé."

# --- Aide ---
help:
	@echo "Usage :"
	@echo "  make         : Compile tout (C et Java)"
	@echo "  make clean   : Supprime les exécutables et les .class"