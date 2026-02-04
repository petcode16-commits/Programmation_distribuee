#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include "msg_temperature.h"

#define MAX_CLIENTS 128
#define UDP_BRIDGE_PORT 9090

// ---------------------------------------------------------
// 1. STRUCTURES ET ETAT GLOBAL
// ---------------------------------------------------------
typedef enum { TYPE_UNKNOWN, TYPE_THERMO, TYPE_CHAUF, TYPE_CTRL, TYPE_CMD } ClientType;

typedef struct {
    int fd;
    ClientType type;
    char piece[64];
} Client;

typedef struct {
    char piece[64];
    int temperature;
} RoomState;

Client clients[MAX_CLIENTS];
RoomState rooms[MAX_CLIENTS];
int num_rooms = 0;

// Utilitaire pour retrouver l'index d'une pièce
int get_room_index(const char *name) {
    for (int i = 0; i < num_rooms; i++) {
        if (strcmp(rooms[i].piece, name) == 0) return i;
    }
    if (num_rooms < MAX_CLIENTS) {
        strncpy(rooms[num_rooms].piece, name, 63);
        rooms[num_rooms].temperature = 0;
        return num_rooms++;
    }
    return 0;
}

// ---------------------------------------------------------
// 2. LOGIQUE D'ENVOI D'ORDRES (Dispatching)
// ---------------------------------------------------------
void forward_to_chauffage(const char *piece, int puissance) {
    char msg[128];
    // On prépare le message texte que le module chauffage.c attend
    snprintf(msg, sizeof(msg), "SET %s %d\n", piece, puissance);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        // On cherche le client qui est un CHAUFFAGE et qui gère la bonne PIECE
        if (clients[i].fd != -1 && clients[i].type == TYPE_CHAUF && strcmp(clients[i].piece, piece) == 0) {
            send(clients[i].fd, msg, strlen(msg), 0);
            printf("[Central] Ordre envoyé -> Module %s : %d\n", piece, puissance);
        }
    }
}

// ---------------------------------------------------------
// 3. MAIN : LA BOUCLE DE MULTIPLEXAGE
// ---------------------------------------------------------
int main(int argc, char **argv) {
    int port_tcp = (argc > 1) ? atoi(argv[1]) : 8080;
    for (int i = 0; i < MAX_CLIENTS; i++) clients[i].fd = -1;

    // -- INIT SOCKETS (TCP pour les modules et UDP pour Java/RMI) --
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr_tcp = { .sin_family = AF_INET, .sin_port = htons(port_tcp), .sin_addr.s_addr = INADDR_ANY };
    int opt = 1; setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    bind(listen_fd, (struct sockaddr*)&addr_tcp, sizeof(addr_tcp));
    listen(listen_fd, 10);

    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr_udp = { .sin_family = AF_INET, .sin_port = htons(UDP_BRIDGE_PORT), .sin_addr.s_addr = INADDR_ANY };
    bind(udp_fd, (struct sockaddr*)&addr_udp, sizeof(addr_udp));

    fd_set master_fds;
    FD_ZERO(&master_fds);
    FD_SET(listen_fd, &master_fds);
    FD_SET(udp_fd, &master_fds);
    int fd_max = (listen_fd > udp_fd) ? listen_fd : udp_fd;

    printf("[Central] Serveur lancé (TCP %d, UDP %d)\n", port_tcp, UDP_BRIDGE_PORT);

    while (1) {
        fd_set read_fds = master_fds;
        select(fd_max + 1, &read_fds, NULL, NULL, NULL);

        for (int i = 0; i <= fd_max; i++) {
            if (FD_ISSET(i, &read_fds)) {
                
                // CAS A : Nouveau client TCP (Thermo, Chauffage ou Console)
                if (i == listen_fd) {
                    int new_c = accept(listen_fd, NULL, NULL);
                    FD_SET(new_c, &master_fds);
                    if (new_c > fd_max) fd_max = new_c;
                    for (int j=0; j<MAX_CLIENTS; j++) if (clients[j].fd == -1) { clients[j].fd = new_c; break; }
                } 
                
                // CAS B : Message UDP (Vient de la console RMI via le pont Java)
                else if (i == udp_fd) {
                    char buf[512]; struct sockaddr_in c_addr; socklen_t len = sizeof(c_addr);
                    int n = recvfrom(udp_fd, buf, 511, 0, (struct sockaddr*)&c_addr, &len);
                    if (n > 0) {
                        buf[n] = '\0';
                        if (strncmp(buf, "LIST", 4) == 0) {
                            char resp[1024] = "";
                            for (int j=0; j<num_rooms; j++) {
                                char tmp[128]; snprintf(tmp, 128, "Pièce %s : %d°C\n", rooms[j].piece, rooms[j].temperature);
                                strcat(resp, tmp);
                            }
                            sendto(udp_fd, resp, strlen(resp), 0, (struct sockaddr*)&c_addr, len);
                        } else if (strncmp(buf, "CMD", 3) == 0) {
                            char p[64]; int v; sscanf(buf + 4, "%s %d", p, &v);
                            forward_to_chauffage(p, v);
                        }
                    }
                } 
                
                // CAS C : Données d'un client TCP déjà connecté
                else {
                    char buf[256]; int n = recv(i, buf, 255, 0);
                    if (n <= 0) { // Déconnexion
                        close(i); FD_CLR(i, &master_fds);
                        for (int j=0; j<MAX_CLIENTS; j++) if (clients[j].fd == i) clients[j].fd = -1;
                    } else {
                        buf[n] = '\0';
                        
                        // 1. Identification du type de client
                        if (strncmp(buf, "THERMO", 6) == 0) {
                            for(int j=0; j<MAX_CLIENTS; j++) if(clients[j].fd == i) {
                                clients[j].type = TYPE_THERMO; sscanf(buf+7, "%s", clients[j].piece);
                                printf("[Central] Identifié : THERMO sur %s\n", clients[j].piece);
                            }
                        } else if (strncmp(buf, "CHAUF", 5) == 0) {
                            for(int j=0; j<MAX_CLIENTS; j++) if(clients[j].fd == i) {
                                clients[j].type = TYPE_CHAUF; sscanf(buf+6, "%s", clients[j].piece);
                                printf("[Central] Identifié : CHAUFFAGE sur %s\n", clients[j].piece);
                            }
                        } else if (strncmp(buf, "CONSOLE_CTRL", 12) == 0) {
                            for(int j=0; j<MAX_CLIENTS; j++) if(clients[j].fd == i) clients[j].type = TYPE_CTRL;
                        } else if (strncmp(buf, "CONSOLE_CMD", 11) == 0) {
                            for(int j=0; j<MAX_CLIENTS; j++) if(clients[j].fd == i) clients[j].type = TYPE_CMD;
                        } 
                        
                        // 2. Traitement des messages
                        // Si c'est une température envoyée par un Thermo
                        else if (strstr(buf, "TEMP") != NULL) {
                            char p[64]; int v;
                            if (sscanf(buf, "TEMP %s %d", p, &v) == 2) {
                                rooms[get_room_index(p)].temperature = v;
                                // On diffuse l'info aux consoles de monitoring
                                for(int k=0; k<MAX_CLIENTS; k++) 
                                    if(clients[k].fd != -1 && clients[k].type == TYPE_CTRL) 
                                        dprintf(clients[k].fd, "UPDATE %s %d\n", p, v);
                            }
                        }
                        // Si c'est un ordre envoyé par la console CMD
                        else if (strstr(buf, "SET") != NULL) {
                            char p[64]; int v;
                            if (sscanf(buf, "SET %s %d", p, &v) == 2) {
                                forward_to_chauffage(p, v);
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}