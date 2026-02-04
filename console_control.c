#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char **argv) {
    if (argc < 3) return 1;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr = { .sin_family = AF_INET, .sin_port = htons(atoi(argv[2])) };
    inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) return 1;

    // S'identifier comme moniteur
    send(sock, "CONSOLE_CTRL\n", 13, 0);
    printf("--- MONITORING TEMPERATURES (Temps Réel) ---\n");

    while (1) {
        char buf[256];
        int n = recv(sock, buf, 255, 0);
        if (n <= 0) break;
        buf[n] = '\0';
        
        // Affiche uniquement les UPDATE
        if (strncmp(buf, "UPDATE", 6) == 0) {
            printf("[Affichage] %s", buf + 7);
        }
    }
    return 0;
}