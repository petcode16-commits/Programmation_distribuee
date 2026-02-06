#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUF_SIZE 256

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <host_central> <port_tcp>\n", argv[0]);
        exit(1);
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));

    // Résolution d'adresse plus robuste
    if (strcmp(argv[1], "localhost") == 0) {
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    } else {
        addr.sin_addr.s_addr = inet_addr(argv[1]);
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Connexion au Central échouée");
        exit(1);
    }

    dprintf(sock, "CONSOLE\n");

    printf("====================================================\n");
    printf("   MONITORING DOMOTIQUE (127.0.0.1)\n");
    printf("====================================================\n");

    char buf[BUF_SIZE];
    while (1) {
        int n = read(sock, buf, BUF_SIZE - 1);
        if (n <= 0) break;
        buf[n] = '\0';

        char piece[32], statut[32];
        int valeur;

        if (sscanf(buf, "UPDATE %s %d", piece, &valeur) == 2) {
            printf("🌡️  [%-10s] Température : %2d°C\n", piece, valeur);
        } else if (sscanf(buf, "STAT %s %s %d", piece, statut, &valeur) == 3) {
            printf("🔥 [%-10s] Chauffage   : %-10s | Pwr: %d\n", piece, statut, valeur);
        }
    }
    close(sock);
    return 0;
}