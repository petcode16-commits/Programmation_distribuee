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
    memset(&addr, 0, sizeof(addr)); // Correction sin_zero warning
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    
    if (inet_pton(AF_INET, argv[1], &addr.sin_addr) <= 0) {
        perror("Adresse invalide");
        exit(1);
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Connexion au Central échouée");
        exit(1);
    }

    printf("==========================================\n");
    printf("   CONSOLE DE COMMANDE (ÉMETTEUR D'ORDRES)\n");
    printf("==========================================\n");
    printf("Formats : \n  - Salon 3 (Manuel)\n  - Salon A 22 (Auto)\n");
    printf("Tapez 'exit' pour quitter.\n\n");

    char line[BUF_SIZE];
    while (1) {
        printf("Commande > ");
        if (!fgets(line, sizeof(line), stdin)) break;
        if (strncmp(line, "exit", 4) == 0) break;

        char piece[32];
        int val;

        // Cas Auto : Piece A 22
        if (sscanf(line, "%s A %d", piece, &val) == 2) {
            dprintf(sock, "AUTO %s %d\n", piece, val);
            printf("✅ Ordre AUTO envoyé (%s -> %d°C)\n", piece, val);
        }
        // Cas Manuel : Piece 3
        else if (sscanf(line, "%s %d", piece, &val) == 2) {
            if (val < 0 || val > 5) {
                printf("❌ Puissance invalide (0-5).\n");
            } else {
                dprintf(sock, "SET %s %d\n", piece, val);
                printf("✅ Ordre MANUEL envoyé (%s -> %d)\n", piece, val);
            }
        } else {
            printf("⚠️ Format : <Piece> <Valeur> ou <Piece> A <Cible>\n");
        }
    }

    close(sock);
    return 0;
}