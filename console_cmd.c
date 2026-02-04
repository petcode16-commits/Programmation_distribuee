#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char **argv) {
    if (argc < 3) return 1;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons(atoi(argv[2])) };
    inet_pton(AF_INET, argv[1], &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) return 1;

    // Envoyer l'ID immédiatement
    send(sock, "CONSOLE_CMD\n", 12, 0);
    printf("Connecté. Tapez: <piece> <valeur>\n");

    char line[256];
    while (printf("> ") && fgets(line, sizeof(line), stdin)) {
        char piece[64]; int val;
        if (sscanf(line, "%s %d", piece, &val) == 2) {
            char cmd[128];
            int len = sprintf(cmd, "SET %s %d\n", piece, val);
            if (send(sock, cmd, len, 0) < 0) break;
            printf("Commande envoyée : %s", cmd);
        }
    }
    close(sock);
    return 0;
}