#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "msg_temperature.h"

// Si struct ip_mreq n'est pas définie par les headers standards
#ifndef IP_ADD_MEMBERSHIP
#define IP_ADD_MEMBERSHIP 35
struct ip_mreq {
    struct in_addr imr_multiaddr; /* IP multicast address of group */
    struct in_addr imr_interface; /* local IP address of interface */
};
#endif

/* Usage: ./thermometre <group_multicast> <port_multi> <nom_piece> <host_central> <port_central> */
int main(int argc, char **argv) {
    if (argc < 6) {
        fprintf(stderr, "Usage: %s <group> <portMulti> <piece> <centralHost> <centralPort>\n", argv[0]);
        return 1;
    }

    const char *group = argv[1];
    int port_multi = atoi(argv[2]);
    const char *nom_piece = argv[3];
    const char *central_host = argv[4];
    int central_port = atoi(argv[5]);

    // 1. Configuration de la socket Multicast (Réception)
    int sock_multi = socket(AF_INET, SOCK_DGRAM, 0);
    int ok = 1;
    setsockopt(sock_multi, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(ok));

    struct sockaddr_in addr_multi;
    memset(&addr_multi, 0, sizeof(addr_multi));
    addr_multi.sin_family = AF_INET;
    addr_multi.sin_port = htons(port_multi);
    addr_multi.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(sock_multi, (struct sockaddr*)&addr_multi, sizeof(addr_multi)) < 0) {
        perror("Echec bind multicast");
        return 1;
    }

    // Jointure du groupe Multicast
    struct ip_mreq mreq;
    inet_pton(AF_INET, group, &mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sock_multi, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("Echec setsockopt IP_ADD_MEMBERSHIP");
        // On continue quand même, parfois c'est juste une restriction réseau
    }

    int central_fd = -1;
    printf("[Thermometre %s] Ecoute sur %s:%d...\n", nom_piece, group, port_multi);

    while (1) {
        unsigned char buf[1024];
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        int n = recvfrom(sock_multi, buf, sizeof(buf), 0, (struct sockaddr*)&from, &fromlen);

        if (n > 0) {
            struct msg_temperature msg;
            if (mt_parse(buf, n, &msg) == 0) {
                if (msg.type == MT_MESURE && strcmp(msg.piece, nom_piece) == 0) {
                    
                    if (central_fd < 0) {
                        central_fd = socket(AF_INET, SOCK_STREAM, 0);
                        struct sockaddr_in serv_addr = { .sin_family = AF_INET, .sin_port = htons(central_port) };
                        inet_pton(AF_INET, central_host, &serv_addr.sin_addr);
                        
                        if (connect(central_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
                            perror("Erreur connexion Central");
                            close(central_fd); central_fd = -1;
                        } else {
                            dprintf(central_fd, "THERMO %s\n", nom_piece);
                        }
                    }

                    if (central_fd >= 0) {
                        if (dprintf(central_fd, "TEMP %s %d\n", nom_piece, msg.valeur) < 0) {
                            close(central_fd); central_fd = -1;
                        }
                    }
                }
                mt_free(&msg);
            }
        }
    }
    return 0;
}