#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 256

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <host_central> <port_tcp>\n", argv[0]);
        exit(1);
    }

    // 1. Création de la socket TCP
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Erreur création socket");
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &addr.sin_addr) <= 0) {
        perror("Adresse invalide");
        exit(1);
    }

    // 2. Connexion au Central
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Connexion au Central échouée");
        exit(1);
    }

    // 3. IDENTIFICATION (Crucial pour recevoir les données)
    // On envoie "CONSOLE\n" pour que le Central nous enregistre comme afficheur
    dprintf(sock, "CONSOLE\n");

    printf("====================================================\n");
    printf("   TABLEAU DE BORD (MONITORING)\n");
    printf("====================================================\n");
    printf("[INFO] Connecté au Central. En attente de données...\n\n");

    char buf[BUF_SIZE];
    while (1) {
        // 4. Lecture des messages relayés par le Central
        int n = read(sock, buf, BUF_SIZE - 1);
        if (n <= 0) {
            printf("\n[ALERTE] Connexion perdue avec le Central.\n");
            break;
        }
        buf[n] = '\0';

        char piece[32], statut[32];
        int valeur;

        // Cas A : Mise à jour de température (Format: UPDATE Salon 22)
        if (sscanf(buf, "UPDATE %s %d", piece, &valeur) == 2) {
            printf("🌡️  [%-10s] Température : %2d°C\n", piece, valeur);
        }
        // Cas B : Changement d'état du chauffage (Format: STAT Salon AUTO_ON 3)
        else if (sscanf(buf, "STAT %s %s %d", piece, statut, &valeur) == 3) {
            printf("🔥 [%-10s] Chauffage   : %-10s | Puissance : %d\n", piece, statut, valeur);
        }
        // Cas C : Message d'erreur du Central
        else if (strncmp(buf, "ERROR", 5) == 0) {
            printf("❌ [ERREUR] %s", buf);
        }
    }

    close(sock);
    return 0;
}