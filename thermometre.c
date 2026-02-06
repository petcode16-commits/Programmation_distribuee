#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "msg_temperature.h"

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <mcast_addr> <port> <piece> <host_central> <port_central>\n", argv[0]);
        exit(1);
    }

    // Connexion TCP au Central
    int sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr_c;
    addr_c.sin_family = AF_INET;
    addr_c.sin_port = htons(atoi(argv[5]));
    inet_pton(AF_INET, argv[4], &addr_c.sin_addr);
    connect(sock_tcp, (struct sockaddr *)&addr_c, sizeof(addr_c));

    // Identification
    dprintf(sock_tcp, "THERMO %s\n", argv[3]);

    // Setup Multicast
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

    unsigned char buf[256];
    while(1) {
        int n = recv(sock_m, buf, sizeof(buf), 0);
        struct msg_temperature msg;
        if (mt_parse(buf, n, &msg) == 0) {
            if (msg.type == MT_MESURE && strcmp(msg.piece, argv[3]) == 0) {
                dprintf(sock_tcp, "TEMP %s %d\n", msg.piece, msg.valeur);
            }
            mt_free(&msg);
        }
    }
    return 0;
}