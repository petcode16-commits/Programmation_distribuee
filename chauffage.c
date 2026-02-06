#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "msg_temperature.h"

int main(int argc, char *argv[]) {
    char *nom_piece = argv[3];
    int sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr_c;
    addr_c.sin_family = AF_INET;
    addr_c.sin_port = htons(atoi(argv[5]));
    inet_pton(AF_INET, argv[4], &addr_c.sin_addr);
    connect(sock_tcp, (struct sockaddr *)&addr_c, sizeof(addr_c));

    dprintf(sock_tcp, "CHAUFFAGE %s\n", nom_piece);

    int sock_m = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr_m = {0};
    addr_m.sin_family = AF_INET;
    addr_m.sin_port = htons(atoi(argv[2]));
    addr_m.sin_addr.s_addr = INADDR_ANY;
    bind(sock_m, (struct sockaddr *)&addr_m, sizeof(addr_m));

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(argv[1]);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(sock_m, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    int puissance = 0, mode_auto = 0, cible = 20;
    fd_set rfds;

    while(1) {
        FD_ZERO(&rfds);
        FD_SET(sock_tcp, &rfds);
        FD_SET(sock_m, &rfds);
        select(FD_SETSIZE, &rfds, NULL, NULL, NULL);

        if (FD_ISSET(sock_tcp, &rfds)) {
            char b[256]; int n = read(sock_tcp, b, 255);
            if (n <= 0) break;
            b[n] = '\0';
            if (sscanf(b, "SET %*s %d", &puissance) == 1) mode_auto = 0;
            if (sscanf(b, "AUTO %*s %d", &cible) == 1) mode_auto = 1;
            dprintf(sock_tcp, "STAT %s %s %d\n", nom_piece, mode_auto?"AUTO":"MANU", puissance);
        }

        if (FD_ISSET(sock_m, &rfds)) {
            unsigned char bm[256];
            int n = recv(sock_m, bm, 256, 0);
            struct msg_temperature msg;
            if (mt_parse(bm, n, &msg) == 0) {
                if (msg.type == MT_MESURE && mode_auto) {
                    puissance = (msg.valeur < cible) ? 3 : 0;
                }
                mt_free(&msg);
            }
            // Envoi de la puissance à l'Air
            struct msg_temperature out = {puissance, MT_CHAUFFER, nom_piece};
            size_t out_len;
            unsigned char *out_buf = mt_serialize(&out, &out_len);
            struct sockaddr_in dest = {0};
            dest.sin_family = AF_INET;
            dest.sin_port = htons(atoi(argv[2]));
            dest.sin_addr.s_addr = inet_addr(argv[1]);
            sendto(sock_m, out_buf, out_len, 0, (struct sockaddr*)&dest, sizeof(dest));
            free(out_buf);
        }
    }
    return 0;
}