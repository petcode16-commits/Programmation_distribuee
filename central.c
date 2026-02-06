#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <signal.h>

#define MAX_CLIENTS 20
#define UDP_PORT_LISTEN 9090 // Ecoute les ordres venant de RMI
#define UDP_PORT_RMI_UPDATE 9091 // Envoie les infos vers RMIServer

// Types de modules
#define TYPE_THERMO 1
#define TYPE_CHAUF  2
#define TYPE_CTRL   3

typedef struct {
    int fd;
    char nom[32];
    int type;
} Client;

Client clients[MAX_CLIENTS];

// Trouver le descripteur d'un chauffage par son nom
int trouver_chauffage(char *nom) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1 && clients[i].type == TYPE_CHAUF && strcmp(clients[i].nom, nom) == 0) {
            return clients[i].fd;
        }
    }
    return -1;
}

// Envoyer un message à toutes les consoles C connectées
void diffuser_aux_consoles(char *msg) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1 && clients[i].type == TYPE_CTRL) {
            send(clients[i].fd, msg, strlen(msg), 0);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port_tcp>\n", argv[0]);
        exit(1);
    }

    int port_tcp = atoi(argv[1]);
    signal(SIGPIPE, SIG_IGN); // Empêche le crash si un module se déconnecte

    for (int i = 0; i < MAX_CLIENTS; i++) clients[i].fd = -1;

    // --- SETUP SOCKET TCP (Serveur pour modules C) ---
    int master_socket = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_tcp);
    bind(master_socket, (struct sockaddr *)&address, sizeof(address));
    listen(master_socket, 5);

    // --- SETUP SOCKET UDP (Ecoute ordres Java + Envoi maj Java) ---
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in udp_addr_listen;
    udp_addr_listen.sin_family = AF_INET;
    udp_addr_listen.sin_addr.s_addr = INADDR_ANY;
    udp_addr_listen.sin_port = htons(UDP_PORT_LISTEN);
    bind(udp_socket, (struct sockaddr *)&udp_addr_listen, sizeof(udp_addr_listen));

    // Adresse de destination pour les mises à jour RMI
    struct sockaddr_in rmi_update_addr;
    rmi_update_addr.sin_family = AF_INET;
    rmi_update_addr.sin_port = htons(UDP_PORT_RMI_UPDATE);
    inet_pton(AF_INET, "127.0.0.1", &rmi_update_addr.sin_addr);

    printf("[CENTRAL] Serveur actif (TCP:%d, UDP_In:%d, UDP_Out_RMI:%d)\n", 
            port_tcp, UDP_PORT_LISTEN, UDP_PORT_RMI_UPDATE);

    fd_set readfds;
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(master_socket, &readfds);
        FD_SET(udp_socket, &readfds);
        int max_sd = (master_socket > udp_socket) ? master_socket : udp_socket;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd > 0) FD_SET(clients[i].fd, &readfds);
            if (clients[i].fd > max_sd) max_sd = clients[i].fd;
        }

        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) continue;

        // --- NOUVELLE CONNEXION TCP ---
        if (FD_ISSET(master_socket, &readfds)) {
            int new_socket = accept(master_socket, NULL, NULL);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == -1) { clients[i].fd = new_socket; break; }
            }
        }

        // --- ORDRES UDP (Venant de ConsoleRMI/RMIServer) ---
        if (FD_ISSET(udp_socket, &readfds)) {
            char buffer[256];
            int n = recvfrom(udp_socket, buffer, sizeof(buffer)-1, 0, NULL, NULL);
            buffer[n] = '\0';
            
            char piece[32];
            if (sscanf(buffer, "%*s %s", piece) >= 1) {
                int fd_chauf = trouver_chauffage(piece);
                if (fd_chauf != -1) send(fd_chauf, buffer, strlen(buffer), 0);
            }
        }

        // --- TRAITEMENT DES MODULES C CONNECTÉS ---
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].fd;
            if (sd != -1 && FD_ISSET(sd, &readfds)) {
                char buffer[256];
                int val_read = read(sd, buffer, sizeof(buffer)-1);

                if (val_read <= 0) { // Déconnexion propre
                    close(sd); clients[i].fd = -1;
                } else {
                    buffer[val_read] = '\0';

                    // 1. Enregistrement initial
                    if (strncmp(buffer, "THERMO", 6) == 0) {
                        clients[i].type = TYPE_THERMO; sscanf(buffer, "THERMO %s", clients[i].nom);
                    } 
                    else if (strncmp(buffer, "CHAUFFAGE", 9) == 0) {
                        clients[i].type = TYPE_CHAUF; sscanf(buffer, "CHAUFFAGE %s", clients[i].nom);
                    }
                    else if (strncmp(buffer, "CONSOLE", 7) == 0) {
                        clients[i].type = TYPE_CTRL; strcpy(clients[i].nom, "Console");
                    }
                    // 2. Relais Temperature (TEMP) et Statut (STAT)
                    else if (strncmp(buffer, "TEMP", 4) == 0 || strncmp(buffer, "STAT", 4) == 0) {
                        // Relais aux consoles C
                        diffuser_aux_consoles(buffer);
                        // Relais au serveur RMI pour mettre à jour son cache local
                        sendto(udp_socket, buffer, strlen(buffer), 0, 
                               (struct sockaddr *)&rmi_update_addr, sizeof(rmi_update_addr));
                    }
                    // 3. Relais Commandes (SET/AUTO)
                    else if (strncmp(buffer, "SET", 3) == 0 || strncmp(buffer, "AUTO", 4) == 0) {
                        char piece_cible[32]; sscanf(buffer, "%*s %s", piece_cible);
                        int fd_dest = trouver_chauffage(piece_cible);
                        if (fd_dest != -1) send(fd_dest, buffer, strlen(buffer), 0);
                        else dprintf(sd, "ERROR Piece %s non connectee\n", piece_cible);
                    }
                }
            }
        }
    }
    return 0;
}