#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>

/* Structure binaire identique à celle utilisée dans Air.java */
typedef struct {
    int temperature;
} msg_physique;

/* Structure pour envoyer la puissance à la simulation */
typedef struct {
    int puissance;
} msg_chauffage;

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <group_multi> <port_multi> <nom_piece> <host_central> <port_central>\n", argv[0]);
        exit(1);
    }

    char *group_multi = argv[1];
    int port_multi = atoi(argv[2]);
    char *nom_piece = argv[3];
    char *host_central = argv[4];
    int port_central = atoi(argv[5]);

    int target_temp = -1; // -1 = Manuel, > 0 = Mode Automatique
    int puissance_actuelle = 0;
    int derniere_puissance_envoyee = -1;

    // --- 1. CONFIGURATION TCP (VERS LE CENTRAL) ---
    int sock_central = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr_central;
    addr_central.sin_family = AF_INET;
    addr_central.sin_port = htons(port_central);
    inet_pton(AF_INET, host_central, &addr_central.sin_addr);

    if (connect(sock_central, (struct sockaddr *)&addr_central, sizeof(addr_central)) < 0) {
        perror("Erreur connexion Central");
        exit(1);
    }
    // Enregistrement auprès du Central
    dprintf(sock_central, "CHAUFFAGE %s\n", nom_piece);

    // --- 2. CONFIGURATION UDP MULTICAST (VERS LA PHYSIQUE) ---
    int sock_multi = socket(AF_INET, SOCK_DGRAM, 0);
    int ok = 1;
    setsockopt(sock_multi, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(ok));

    struct sockaddr_in addr_multi;
    memset(&addr_multi, 0, sizeof(addr_multi));
    addr_multi.sin_family = AF_INET;
    addr_multi.sin_port = htons(port_multi);
    addr_multi.sin_addr.s_addr = inet_addr(group_multi);

    // Bind pour recevoir les températures
    bind(sock_multi, (struct sockaddr *)&addr_multi, sizeof(addr_multi));

    // Join le groupe Multicast
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(group_multi);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(sock_multi, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    printf("Module Chauffage [%s] démarré (Mode Manuel).\n", nom_piece);

    // --- 3. BOUCLE PRINCIPALE AVEC SELECT ---
    fd_set readfds;
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sock_central, &readfds);
        FD_SET(sock_multi, &readfds);
        int max_fd = (sock_central > sock_multi) ? sock_central : sock_multi;

        if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("Erreur select");
            break;
        }

        // A. RÉCEPTION D'UN ORDRE DU CENTRAL (TCP)
        if (FD_ISSET(sock_central, &readfds)) {
            char buf[128];
            int n = read(sock_central, buf, sizeof(buf) - 1);
            if (n <= 0) {
                printf("Fermeture du Central. Arrêt.\n");
                break;
            }
            buf[n] = '\0';

            if (strncmp(buf, "AUTO", 4) == 0) {
                sscanf(buf, "AUTO %*s %d", &target_temp);
                printf("[%s] Passage en mode AUTO (Cible: %d°C)\n", nom_piece, target_temp);
            } 
            else if (strncmp(buf, "SET", 3) == 0) {
                target_temp = -1; // Désactive le mode Auto
                sscanf(buf, "SET %*s %d", &puissance_actuelle);
                printf("[%s] Mode MANUEL (Puissance: %d)\n", nom_piece, puissance_actuelle);
                
                // Envoi immédiat de la puissance à la simulation
                msg_chauffage mc = { puissance_actuelle };
                sendto(sock_multi, &mc, sizeof(mc), 0, (struct sockaddr *)&addr_multi, sizeof(addr_multi));
                dprintf(sock_central, "STAT %s ON %d\n", nom_piece, puissance_actuelle);
            }
        }

        // B. RÉCEPTION DE LA TEMPÉRATURE (MULTICAST) ET RÉGULATION
        if (FD_ISSET(sock_multi, &readfds)) {
            msg_physique mp;
            recv(sock_multi, &mp, sizeof(mp), 0);

            if (target_temp != -1) { // On est en mode AUTOMATIQUE
                // Logique de régulation simple (Hystérésis)
                if (mp.temperature < target_temp) {
                    puissance_actuelle = 15; // Allumer
                } else {
                    puissance_actuelle = 0;  // Éteindre
                }

                // Envoyer à la simulation seulement si la puissance change
                if (puissance_actuelle != derniere_puissance_envoyee) {
                    msg_chauffage mc = { puissance_actuelle };
                    sendto(sock_multi, &mc, sizeof(mc), 0, (struct sockaddr *)&addr_multi, sizeof(addr_multi));
                    
                    // Informer le Central pour l'affichage console
                    dprintf(sock_central, "STAT %s %s %d\n", 
                            nom_piece, 
                            (puissance_actuelle > 0 ? "AUTO_ON" : "AUTO_OFF"), 
                            puissance_actuelle);
                    
                    derniere_puissance_envoyee = puissance_actuelle;
                }
            }
        }
    }

    close(sock_central);
    close(sock_multi);
    return 0;
}