#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include "msg_temperature.h"

#define MAX_BUF 256

int main(int argc, char *argv[]) {
    // Correction du warning unused parameter 'argc' par une vérification de sécurité
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <addr_multi> <port_multi> <nom_piece> <host_central> <port_central>\n", argv[0]);
        exit(1);
    }

    char *addr_multi = argv[1];
    int port_multi = atoi(argv[2]);
    char *nom_piece = argv[3];
    char *host_central = argv[4];
    int port_central = atoi(argv[5]);

    int puissance = 0;       
    int temp_cible = 20;     
    int mode_auto = 0;       

    // --- 1. SETUP MULTICAST (Écoute de l'Air) ---
    int sock_m = socket(AF_INET, SOCK_DGRAM, 0);
    int reuse = 1;
    setsockopt(sock_m, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr_m;
    memset(&addr_m, 0, sizeof(addr_m)); // Nettoyage structure
    addr_m.sin_family = AF_INET;
    addr_m.sin_port = htons(port_multi);
    addr_m.sin_addr.s_addr = INADDR_ANY;
    bind(sock_m, (struct sockaddr *)&addr_m, sizeof(addr_m));

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(addr_multi);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(sock_m, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    // --- 2. SETUP TCP (Connexion au Central) ---
    int sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr_c;
    memset(&addr_c, 0, sizeof(addr_c)); // Nettoyage structure
    addr_c.sin_family = AF_INET;
    addr_c.sin_port = htons(port_central);
    inet_pton(AF_INET, host_central, &addr_c.sin_addr);

    if (connect(sock_tcp, (struct sockaddr *)&addr_c, sizeof(addr_c)) < 0) {
        perror("Échec connexion Central");
        exit(1);
    }

    // Identification auprès du Central
    dprintf(sock_tcp, "CHAUFFAGE %s\n", nom_piece);
    printf("[CHAUFFAGE] Module %s connecté au Central.\n", nom_piece);

    fd_set readfds;
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sock_m, &readfds);
        FD_SET(sock_tcp, &readfds);
        int max_fd = (sock_m > sock_tcp) ? sock_m : sock_tcp;

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) continue;

        // --- RÉCEPTION BINAIRE DEPUIS L'AIR (MULTICAST) ---
        if (FD_ISSET(sock_m, &readfds)) {
            unsigned char buf_m[MAX_BUF];
            int n = recv(sock_m, buf_m, sizeof(buf_m), 0);
            struct msg_temperature msg;
            
            if (mt_parse(buf_m, n, &msg) == 0) {
                // Si c'est une mesure de notre pièce et qu'on est en mode AUTO
                if (msg.type == MT_MESURE && strcmp(msg.piece, nom_piece) == 0 && mode_auto) {
                    int ancienne_pwr = puissance;
                    puissance = (msg.valeur < temp_cible) ? 3 : 0;

                    if (puissance != ancienne_pwr) {
                        dprintf(sock_tcp, "STAT %s AUTO_%s %d\n", nom_piece, (puissance>0?"ON":"OFF"), puissance);
                    }
                }
                mt_free(&msg);
            }

            // Envoi de la puissance actuelle vers l'Air (Format Binaire)
            struct msg_temperature out = {puissance, MT_CHAUFFER, nom_piece};
            size_t out_len;
            unsigned char *out_buf = mt_serialize(&out, &out_len);
            
            struct sockaddr_in dest_m;
            memset(&dest_m, 0, sizeof(dest_m));
            dest_m.sin_family = AF_INET;
            dest_m.sin_port = htons(port_multi);
            dest_m.sin_addr.s_addr = inet_addr(addr_multi);
            
            sendto(sock_m, out_buf, out_len, 0, (struct sockaddr*)&dest_m, sizeof(dest_m));
            free(out_buf);
        }

        // --- RÉCEPTION COMMANDES DEPUIS LE CENTRAL (TCP) ---
        if (FD_ISSET(sock_tcp, &readfds)) {
            char b[MAX_BUF];
            int n = read(sock_tcp, b, MAX_BUF - 1);
            if (n <= 0) {
                printf("[INFO] Déconnexion du Central. Fin du module.\n");
                break;
            }
            b[n] = '\0';

            int val;
            if (sscanf(b, "SET %*s %d", &val) == 1) {
                mode_auto = 0;
                puissance = val;
                printf("[%s] Mode MANU: Puissance %d\n", nom_piece, puissance);
                dprintf(sock_tcp, "STAT %s MANU %d\n", nom_piece, puissance);
            } 
            else if (sscanf(b, "AUTO %*s %d", &val) == 1) {
                mode_auto = 1;
                temp_cible = val;
                printf("[%s] Mode AUTO: Cible %d°C\n", nom_piece, temp_cible);
                dprintf(sock_tcp, "STAT %s AUTO_START %d\n", nom_piece, puissance);
            }
        }
    }

    close(sock_tcp);
    close(sock_m);
    return 0;
}