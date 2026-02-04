#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "msg_temperature.h"

int main(int argc, char **argv) {
    if (argc < 6) {
        fprintf(stderr, "Usage: %s <group> <portMulti> <piece> <hostCentral> <portCentral>\n", argv[0]);
        return 1;
    }

    const char *group = argv[1];
    int port_multi = atoi(argv[2]);
    const char *nom_piece = argv[3];
    const char *central_host = argv[4];
    int central_port = atoi(argv[5]);

    // 1. Socket Multicast pour la simulation Air.java
    int sock_multi = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr_multi = { .sin_family = AF_INET, .sin_port = htons(port_multi) };
    inet_pton(AF_INET, group, &addr_multi.sin_addr);

    // 2. Socket TCP pour recevoir les ordres du Central
    int sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr = { .sin_family = AF_INET, .sin_port = htons(central_port) };
    inet_pton(AF_INET, central_host, &serv_addr.sin_addr);

    if (connect(sock_tcp, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connexion Central échouée");
        return 1;
    }

    // Identification
    dprintf(sock_tcp, "CHAUF %s\n", nom_piece);
    printf("[Chauffage %s] Prêt et connecté au Central.\n", nom_piece);

    while (1) {
        char buf[256];
        int n = recv(sock_tcp, buf, 255, 0);
        if (n <= 0) break;
        buf[n] = '\0';

        // Si le Central envoie "SET <piece> <valeur>"
        if (strncmp(buf, "SET", 3) == 0) {
            int puissance;
            sscanf(buf + 4, "%*s %d", &puissance);

            // Création du message conforme au sujet
            struct msg_temperature m;
            m.type = MT_CHAUFFER; // Utilisation de ta constante (1)
            m.valeur = puissance;
            m.piece = (char *)nom_piece;

            // Sérialisation binaire
            size_t out_len;
            unsigned char *out_buf = mt_serialize(&m, &out_len);
            
            if (out_buf) {
                sendto(sock_multi, out_buf, out_len, 0, (struct sockaddr*)&addr_multi, sizeof(addr_multi));
                free(out_buf); // Important : mt_serialize alloue de la mémoire
                printf("[Chauffage %s] Ordre de chauffe envoyé : %d\n", nom_piece, puissance);
            }
        }
    }
    close(sock_tcp);
    return 0;
}