#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "msg_temperature.h"

int main(int argc, char *argv[]) {
    // 1. Vérification des arguments (supprime le warning unused parameter)
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <addr_multi> <port_multi> <piece> <host_central> <port_central>\n", argv[0]);
        exit(1);
    }

    char *addr_multi = argv[1];
    int port_multi = atoi(argv[2]);
    char *nom_piece = argv[3];
    char *host_central = argv[4];
    int port_central = atoi(argv[5]);

    // --- 2. CONNEXION TCP AU CENTRAL ---
    int sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr_c;
    memset(&addr_c, 0, sizeof(addr_c)); // Suppression warning sin_zero
    addr_c.sin_family = AF_INET;
    addr_c.sin_port = htons(port_central);
    
    // Résolution d'adresse robuste (localhost -> 127.0.0.1)
    if (strcmp(host_central, "localhost") == 0) {
        addr_c.sin_addr.s_addr = inet_addr("127.0.0.1");
    } else {
        addr_c.sin_addr.s_addr = inet_addr(host_central);
    }

    if (connect(sock_tcp, (struct sockaddr *)&addr_c, sizeof(addr_c)) < 0) {
        perror("Connexion au Central échouée (Thermo)");
        exit(1);
    }

    // Identification
    dprintf(sock_tcp, "THERMO %s\n", nom_piece);
    printf("[THERMO %s] Connecté au Central.\n", nom_piece);

    // --- 3. SETUP MULTICAST (Écoute de l'Air) ---
    int sock_m = socket(AF_INET, SOCK_DGRAM, 0);
    int reuse = 1;
    setsockopt(sock_m, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr_m;
    memset(&addr_m, 0, sizeof(addr_m));
    addr_m.sin_family = AF_INET;
    addr_m.sin_port = htons(port_multi);
    addr_m.sin_addr.s_addr = INADDR_ANY; 
    
    if (bind(sock_m, (struct sockaddr *)&addr_m, sizeof(addr_m)) < 0) {
        perror("Bind Multicast échoué");
        exit(1);
    }

    // Jointure du groupe Multicast
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(addr_multi);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(sock_m, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    printf("[THERMO %s] Écoute sur %s:%d\n", nom_piece, addr_multi, port_multi);

    // --- 4. BOUCLE DE RELAIS ---
    unsigned char buf[512];
    while(1) {
        int n = recv(sock_m, buf, sizeof(buf), 0);
        if (n < 5) continue; // Un message valide fait au moins 5 octets

        struct msg_temperature msg;
        if (mt_parse(buf, n, &msg) == 0) {
            // On ne relaie QUE les mesures (type 0) de NOTRE pièce
            if (msg.type == MT_MESURE && strcmp(msg.piece, nom_piece) == 0) {
                // Envoi au Central au format texte attendu
                dprintf(sock_tcp, "TEMP %s %d\n", msg.piece, msg.valeur);
                printf(">>> Relais TEMP : %s = %d°C\n", msg.piece, msg.valeur);
            }
            mt_free(&msg); // Libération de la mémoire allouée par mt_parse
        }
    }

    close(sock_tcp);
    close(sock_m);
    return 0;
}