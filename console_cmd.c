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
    if (sock < 0) {
        perror("Erreur lors de la création de la socket");
        exit(1);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    
    // Gestion robuste de localhost
    if (strcmp(argv[1], "localhost") == 0) {
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    } else {
        addr.sin_addr.s_addr = inet_addr(argv[1]);
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Connexion au Central échouée");
        exit(1);
    }

    printf("====================================================\n");
    printf("   CONSOLE DE COMMANDE - RÈGLES DE SAISIE\n");
    printf("====================================================\n");
    printf(" 1. Mode MANUEL : <NomPiece> <0-5>\n");
    printf(" 2. Mode AUTO   : <NomPiece> A <15-30>\n");
    printf("----------------------------------------------------\n");
    printf(" Tapez 'exit' pour quitter.\n\n");

    char line[256];
    while (1) {
        printf("Commande > ");
        if (!fgets(line, sizeof(line), stdin)) break;

        // Suppression du retour à la ligne '\n' capturé par fgets
        line[strcspn(line, "\n")] = 0;

        if (strcmp(line, "exit") == 0) {
            printf("Fermeture de la console...\n");
            break;
        }

        char piece[32];
        int valeur;

        // --- TEST MODE AUTO : <Piece> A <Valeur> ---
        if (sscanf(line, "%s A %d", piece, &valeur) == 2) {
            if (valeur < 15 || valeur > 30) {
                printf("❌ Erreur : Température cible hors limite ! Intervalle autorisé : [15-30] °C.\n");
            } else {
                dprintf(sock, "AUTO %s %d\n", piece, valeur);
                printf("✅ Commande AUTO envoyée : %s réglé sur %d°C.\n", piece, valeur);
            }
        }
        // --- TEST MODE MANUEL : <Piece> <Valeur> ---
        else if (sscanf(line, "%s %d", piece, &valeur) == 2) {
            if (valeur < 0 || valeur > 5) {
                printf("❌ Erreur : Puissance hors limite ! Intervalle autorisé : [0-5].\n");
            } else {
                dprintf(sock, "SET %s %d\n", piece, valeur);
                printf("✅ Commande MANU envoyée : %s réglé à la puissance %d.\n", piece, valeur);
            }
        }
        // --- FORMAT INCONNU ---
        else if (strlen(line) > 0) {
            printf("⚠️ Format invalide. Rappel :\n");
            printf("   - Manuel : Salon 3\n");
            printf("   - Auto   : Salon A 22\n");
        }
    }

    close(sock);
    return 0;
}