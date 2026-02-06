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

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Connexion échouée");
        exit(1);
    }

    // Identification (Optionnel pour les commandes, mais propre)
    dprintf(sock, "CONSOLE_CMD\n");

    printf("==========================================\n");
    printf("   CONSOLE DE COMMANDE (ÉMETTEUR D'ORDRES)\n");
    printf("==========================================\n");
    printf("Formats acceptés :\n");
    printf("  - <Piece> <Puissance(0-5)>  -> ex: Salon 3\n");
    printf("  - <Piece> A <Cible(5-35)>   -> ex: Salon A 21\n");
    printf("  - 'exit' pour quitter\n\n");

    char line[BUF_SIZE];
    while (1) {
        printf("Commande > ");
        if (!fgets(line, sizeof(line), stdin)) break;
        if (strncmp(line, "exit", 4) == 0) break;

        char piece[32];
        char mode[10];
        int val;

        // Cas 1 : Mode AUTO (ex: Salon A 22)
        if (sscanf(line, "%s A %d", piece, &val) == 2) {
            dprintf(sock, "AUTO %s %d\n", piece, val);
            printf("✅ Ordre AUTO envoyé pour %s (Cible: %d°C)\n", piece, val);
        }
        // Cas 2 : Mode MANUEL (ex: Salon 3)
        else if (sscanf(line, "%s %d", piece, &val) == 2) {
            if (val < 0 || val > 5) {
                printf("❌ Erreur : Puissance doit être entre 0 et 5\n");
            } else {
                dprintf(sock, "SET %s %d\n", piece, val);
                printf("✅ Ordre MANUEL envoyé pour %s (Puissance: %d)\n", piece, val);
            }
        } else {
            printf("⚠️ Format invalide.\n");
        }
    }

    close(sock);
    return 0;
}