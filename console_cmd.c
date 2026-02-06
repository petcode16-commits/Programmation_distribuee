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

    // Identification auprès du Central
    dprintf(sock, "CONSOLE\n");

    printf("====================================================\n");
    printf("   CONSOLE DE COMMANDE\n");
    printf("====================================================\n");
    printf("Formats autorisés :\n");
    printf("  1. Manuel : <PIECE> <PUISSANCE> (Puissance: 0 à 5)\n");
    printf("  2. Auto   : <PIECE> AUTO <TEMP> (Temp cible: ex 20)\n");
    printf("----------------------------------------------------\n");

    char line[256];
    while (printf("> ") && fgets(line, sizeof(line), stdin)) {
        char piece[32], mode[16];
        int valeur;

        // Cas 1 : Mode Manuel (Ex: Salon 3)
        if (sscanf(line, "%s %d", piece, &valeur) == 2) {
            // VERIFICATION DE LA PUISSANCE (0 à 5)
            if (valeur < 0 || valeur > 5) {
                printf("❌ Erreur : La puissance doit être comprise entre 0 et 5.\n");
            } else {
                dprintf(sock, "SET %s %d\n", piece, valeur);
                printf("✅ Commande envoyée : %s -> Puissance %d\n", piece, valeur);
            }
        }
        // Cas 2 : Mode AUTO (Ex: Salon AUTO 21)
        else if (sscanf(line, "%s %s %d", piece, mode, &valeur) == 3 && strcmp(mode, "AUTO") == 0) {
            // Ici valeur est une température, on peut aussi mettre une limite (ex: 10 à 30°C)
            if (valeur < 5 || valeur > 35) {
                 printf("❌ Erreur : La température cible doit être entre 5 et 35°C.\n");
            } else {
                dprintf(sock, "AUTO %s %d\n", piece, valeur);
                printf("✅ Mode AUTO activé : %s -> Cible %d°C\n", piece, valeur);
            }
        }
        else {
            printf("⚠️ Format invalide ! Utilisez : <Piece> <0-5> OU <Piece> AUTO <Temp>\n");
        }
    }

    close(sock);
    return 0;
}