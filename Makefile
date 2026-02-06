# --- Configuration du compilateur C ---
CC = gcc
CFLAGS = -Wall -Wextra -g
DEFS = -D_GNU_SOURCE 

# --- Configuration Java ---
JAVAC = javac
# On liste les fichiers Java réels (HomeInterface est le contrat indispensable)
JAVA_SOURCES = MessageTemperature.java Air.java HomeInterface.java RMIServer.java ConsoleRMI.java

# --- Cibles principales ---
all: msg_temperature.o central thermometre chauffage console_control console_cmd compile_java

# 1. Compilation de la bibliothèque de protocole commune (Objet)
msg_temperature.o: msg_temperature.c msg_temperature.h
	$(CC) $(CFLAGS) $(DEFS) -c msg_temperature.c

# 2. Compilation des exécutables C
central: central.c msg_temperature.h
	$(CC) $(CFLAGS) $(DEFS) central.c msg_temperature.o -o central

thermometre: thermometre.c msg_temperature.h
	$(CC) $(CFLAGS) $(DEFS) thermometre.c msg_temperature.o -o thermometre

chauffage: chauffage.c msg_temperature.h
	$(CC) $(CFLAGS) $(DEFS) chauffage.c msg_temperature.o -o chauffage

console_control: console_control.c
	$(CC) $(CFLAGS) $(DEFS) console_control.c -o console_control

console_cmd: console_cmd.c
	$(CC) $(CFLAGS) $(DEFS) console_cmd.c -o console_cmd

# 3. Compilation des fichiers Java
# Utiliser une cible qui vérifie les fichiers sources
compile_java: $(JAVA_SOURCES)
	$(JAVAC) $(JAVA_SOURCES)

# --- Nettoyage ---
clean:
	rm -f *.o central thermometre chauffage console_control console_cmd *.class
	@echo "Nettoyage terminé. Tous les binaires et .class ont été supprimés."

# Cibles "phonies" (ne sont pas des fichiers)
.PHONY: all clean compile_java