#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <signal.h>

#define MAX_C 20
typedef struct { int fd; char nom[32]; int type; } Client;
Client cls[MAX_C];

void diffuse(char *m) {
    for(int i=0; i<MAX_C; i++) {
        if(cls[i].fd != -1 && cls[i].type == 3) {
            send(cls[i].fd, m, strlen(m), 0);
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port_tcp>\n", argv[0]);
        exit(1);
    }

    // Résilience contre les déconnexions brutales
    signal(SIGPIPE, SIG_IGN);

    // Initialisation Socket TCP
    int master = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_port = htons(atoi(argv[1]));
    a.sin_addr.s_addr = INADDR_ANY;

    int opt = 1; 
    setsockopt(master, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    if (bind(master, (struct sockaddr*)&a, sizeof(a)) < 0) {
        perror("Bind TCP échoué");
        exit(1);
    }
    listen(master, 5);

    // Initialisation Socket UDP (Ecoute RMI)
    int udp = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in u_in;
    memset(&u_in, 0, sizeof(u_in));
    u_in.sin_family = AF_INET;
    u_in.sin_port = htons(9090);
    u_in.sin_addr.s_addr = INADDR_ANY;
    bind(udp, (struct sockaddr*)&u_in, sizeof(u_in));

    // Adresse cible pour les mises à jour RMI
    struct sockaddr_in u_rmi;
    memset(&u_rmi, 0, sizeof(u_rmi));
    u_rmi.sin_family = AF_INET;
    u_rmi.sin_port = htons(9091);
    u_rmi.sin_addr.s_addr = inet_addr("127.0.0.1");

    for(int i=0; i<MAX_C; i++) cls[i].fd = -1;

    printf("[CENTRAL] Serveur prêt (TCP:%s UDP:9090)\n", argv[1]);

    while(1) {
        fd_set fds; 
        FD_ZERO(&fds); 
        FD_SET(master, &fds); 
        FD_SET(udp, &fds);
        int m_fd = (master > udp) ? master : udp;

        for(int i=0; i<MAX_C; i++) { 
            if(cls[i].fd != -1) { 
                FD_SET(cls[i].fd, &fds); 
                if(cls[i].fd > m_fd) m_fd = cls[i].fd; 
            }
        }
        
        if (select(m_fd + 1, &fds, NULL, NULL, NULL) < 0) continue;

        // Nouvel arrivant TCP
        if (FD_ISSET(master, &fds)) {
            int n = accept(master, 0, 0);
            int trouve = 0;
            for(int i=0; i<MAX_C; i++) {
                if(cls[i].fd == -1) { 
                    cls[i].fd = n; 
                    trouve = 1;
                    break; 
                }
            }
            if (!trouve) close(n);
        }

        // Commande UDP venant de RMI
        if (FD_ISSET(udp, &fds)) {
            char b[256]; 
            int n = recv(udp, b, 255, 0); 
            if (n > 0) {
                b[n] = '\0';
                char p[32]; 
                if (sscanf(b, "%*s %s", p) == 1) {
                    for(int i=0; i<MAX_C; i++) {
                        if(cls[i].fd != -1 && cls[i].type == 2 && strcmp(cls[i].nom, p) == 0) {
                            send(cls[i].fd, b, n, 0);
                        }
                    }
                }
            }
        }

        // Messages des modules connectés
        for(int i=0; i<MAX_C; i++) {
            if (cls[i].fd != -1 && FD_ISSET(cls[i].fd, &fds)) {
                char b[256]; 
                int n = read(cls[i].fd, b, 255);
                if (n <= 0) { 
                    printf("[LOG] Déconnexion de %s\n", cls[i].nom);
                    close(cls[i].fd); 
                    cls[i].fd = -1; 
                    continue; 
                }
                b[n] = '\0';

                if (strncmp(b, "THERMO", 6) == 0) { 
                    cls[i].type = 1; 
                    sscanf(b, "THERMO %s", cls[i].nom); 
                }
                else if (strncmp(b, "CHAUFFAGE", 9) == 0) { 
                    cls[i].type = 2; 
                    sscanf(b, "CHAUFFAGE %s", cls[i].nom); 
                }
                else if (strncmp(b, "CONSOLE", 7) == 0) { 
                    cls[i].type = 3; 
                    strcpy(cls[i].nom, "Monitor");
                }
                else if (strncmp(b, "TEMP", 4) == 0 || strncmp(b, "STAT", 4) == 0) {
                    char relay[256];
                    if(b[0] == 'T') { 
                        char p[32]; int v; 
                        sscanf(b, "TEMP %s %d", p, &v); 
                        sprintf(relay, "UPDATE %s %d\n", p, v); 
                    } else {
                        strcpy(relay, b);
                    }
                    diffuse(relay);
                    sendto(udp, relay, strlen(relay), 0, (struct sockaddr*)&u_rmi, sizeof(u_rmi));
                }
                else if (strncmp(b, "SET", 3) == 0 || strncmp(b, "AUTO", 4) == 0) {
                    char p[32]; sscanf(b, "%*s %s", p);
                    for(int j=0; j<MAX_C; j++) {
                        if(cls[j].fd != -1 && cls[j].type == 2 && strcmp(cls[j].nom, p) == 0) {
                            send(cls[j].fd, b, strlen(b), 0);
                        }
                    }
                }
            }
        }
    }
    return 0;
}