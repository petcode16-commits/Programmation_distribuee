# --- Configuration du compilateur C ---
CC = gcc
CFLAGS = -Wall -Wextra -g
# On utilise -D_GNU_SOURCE pour éviter les soucis de structures réseaux sur certaines libC
DEFS = -D_GNU_SOURCE 

# --- Configuration Java ---
JAVAC = javac
JAVA_FILES = MessageTemperature.java Air.java RemoteConsole.java RMIServer.java ConsoleRMI.java

# --- Cibles principales ---
all: msg_temperature.o central thermometre chauffage console_control console_cmd compile_java

# 1. Compilation de la bibliothèque de protocole commune (Objet)
msg_temperature.o: msg_temperature.c msg_temperature.h
	$(CC) $(CFLAGS) $(DEFS) -c msg_temperature.c

# 2. Compilation des exécutables C
# Chaque exécutable est lié avec msg_temperature.o pour le protocole
central: central.c msg_temperature.o
	$(CC) $(CFLAGS) $(DEFS) central.c msg_temperature.o -o central

thermometre: thermometre.c msg_temperature.o
	$(CC) $(CFLAGS) $(DEFS) thermometre.c msg_temperature.o -o thermometre

chauffage: chauffage.c msg_temperature.o
	$(CC) $(CFLAGS) $(DEFS) chauffage.c msg_temperature.o -o chauffage

# Consoles C (plus simples, pas besoin de msg_temperature car elles parlent en texte avec le Central)
console_control: console_control.c
	$(CC) $(CFLAGS) $(DEFS) console_control.c -o console_control

console_cmd: console_cmd.c
	$(CC) $(CFLAGS) $(DEFS) console_cmd.c -o console_cmd

# 3. Compilation des fichiers Java
compile_java:
	$(JAVAC) $(JAVA_FILES)

# --- Nettoyage ---
clean:
	rm -f *.o central thermometre chauffage console_control console_cmd *.class
	@echo "Nettoyage terminé."

.PHONY: all clean compile_java