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
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = (strcmp(argv[1], "localhost") == 0) ? inet_addr("127.0.0.1") : inet_addr(argv[1]);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Connexion échouée");
        exit(1);
    }

    char line[256];
    while (1) {
        printf("Commande (ex: Salon 3 ou Salon A 20) > ");
        if (!fgets(line, sizeof(line), stdin)) break;
        char p[32]; int v;
        if (sscanf(line, "%s A %d", p, &v) == 2) dprintf(sock, "AUTO %s %d\n", p, v);
        else if (sscanf(line, "%s %d", p, &v) == 2) dprintf(sock, "SET %s %d\n", p, v);
    }
    close(sock);
    return 0;
}