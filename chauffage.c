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
    // 1. Validation des arguments (Correction unused argc)
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

    // --- 2. SETUP MULTICAST (Écoute & Envoi vers l'Air) ---
    int sock_m = socket(AF_INET, SOCK_DGRAM, 0);
    int reuse = 1;
    setsockopt(sock_m, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr_m;
    memset(&addr_m, 0, sizeof(addr_m));
    addr_m.sin_family = AF_INET;
    addr_m.sin_port = htons(port_multi);
    addr_m.sin_addr.s_addr = INADDR_ANY;
    bind(sock_m, (struct sockaddr *)&addr_m, sizeof(addr_m));

    // Rejoindre le groupe pour écouter les mesures de l'Air
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(addr_multi);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(sock_m, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    // --- 3. SETUP TCP (Connexion au Central) ---
    int sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr_c;
    memset(&addr_c, 0, sizeof(addr_c));
    addr_c.sin_family = AF_INET;
    addr_c.sin_port = htons(port_central);
    
    // Correction de l'adresse (localhost support)
    if (strcmp(host_central, "localhost") == 0) {
        addr_c.sin_addr.s_addr = inet_addr("127.0.0.1");
    } else {
        addr_c.sin_addr.s_addr = inet_addr(host_central);
    }

    if (connect(sock_tcp, (struct sockaddr *)&addr_c, sizeof(addr_c)) < 0) {
        perror("Échec connexion Central (Chauffage)");
        exit(1);
    }

    dprintf(sock_tcp, "CHAUFFAGE %s\n", nom_piece);
    printf("[CHAUFFAGE %s] Prêt.\n", nom_piece);

    fd_set readfds;
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sock_m, &readfds);
        FD_SET(sock_tcp, &readfds);
        int max_fd = (sock_m > sock_tcp) ? sock_m : sock_tcp;

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) continue;

        // --- RÉCEPTION BINAIRE MULTICAST (Air -> Chauffage) ---
        if (FD_ISSET(sock_m, &readfds)) {
            unsigned char buf_m[MAX_BUF];
            int n = recv(sock_m, buf_m, sizeof(buf_m), 0);
            struct msg_temperature msg;
            
            if (mt_parse(buf_m, n, &msg) == 0) {
                // Si on reçoit une mesure de notre pièce et qu'on est en AUTO
                if (msg.type == MT_MESURE && strcmp(msg.piece, nom_piece) == 0 && mode_auto) {
                    int pwr_prec = puissance;
                    puissance = (msg.valeur < temp_cible) ? 3 : 0;
                    if (puissance != pwr_prec) {
                        dprintf(sock_tcp, "STAT %s AUTO_%s %d\n", nom_piece, (puissance>0?"ON":"OFF"), puissance);
                    }
                }
                mt_free(&msg);
            }

            // --- ENVOI BINAIRE MULTICAST (Chauffage -> Air) ---
            struct msg_temperature out = {puissance, MT_CHAUFFER, nom_piece};
            size_t out_len;
            unsigned char *out_buf = mt_serialize(&out, &out_len);
            
            struct sockaddr_in dest_m;
            memset(&dest_m, 0, sizeof(dest_m));
            dest_m.sin_family = AF_INET;
            dest_m.sin_port = htons(port_multi);
            dest_m.sin_addr.s_addr = inet_addr(addr_multi);
            
            // CRUCIAL : Forcer l'interface sur 127.0.0.1 pour que Java reçoive
            struct in_addr localIf;
            localIf.s_addr = inet_addr("127.0.0.1");
            setsockopt(sock_m, IPPROTO_IP, IP_MULTICAST_IF, &localIf, sizeof(localIf));

            sendto(sock_m, out_buf, out_len, 0, (struct sockaddr*)&dest_m, sizeof(dest_m));
            free(out_buf);
        }

        // --- RÉCEPTION COMMANDES TCP (Central -> Chauffage) ---
        if (FD_ISSET(sock_tcp, &readfds)) {
            char b[MAX_BUF];
            int n = read(sock_tcp, b, MAX_BUF - 1);
            if (n <= 0) break;
            b[n] = '\0';

            int val;
            if (sscanf(b, "SET %*s %d", &val) == 1) {
                mode_auto = 0; puissance = val;
                dprintf(sock_tcp, "STAT %s MANU %d\n", nom_piece, puissance);
            } 
            else if (sscanf(b, "AUTO %*s %d", &val) == 1) {
                mode_auto = 1; temp_cible = val;
                dprintf(sock_tcp, "STAT %s AUTO_START %d\n", nom_piece, puissance);
            }
        }
    }
    return 0;
}