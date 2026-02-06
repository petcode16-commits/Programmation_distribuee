#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <host_central> <port_tcp>\n", argv[0]);
        exit(1);
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Connexion au Central échouée");
        exit(1);
    }

    dprintf(sock, "CONSOLE\n");
    printf("[MONITORING] Connecté au Central. En attente de données...\n\n");

    char buf[256];
    while (1) {
        int n = read(sock, buf, sizeof(buf) - 1);
        if (n <= 0) break;
        buf[n] = '\0';

        char piece[32], statut[16];
        int valeur;

        // Réception d'une mise à jour de température
        if (sscanf(buf, "UPDATE %s %d", piece, &valeur) == 2) {
            printf("🌡️  [%s] Température actuelle : %d°C\n", piece, valeur);
        }
        // Réception du statut du chauffage (Mode Auto ou Manuel)
        else if (sscanf(buf, "STAT %s %s %d", piece, statut, &valeur) == 3) {
            printf("🔥 [%s] Chauffage : %s | Puissance : %d\n", piece, statut, valeur);
        }
        // Réception d'une erreur (ex: pièce non trouvée)
        else if (strncmp(buf, "ERROR", 5) == 0) {
            printf("❌ %s", buf);
        }
    }

    close(sock);
    return 0;
}