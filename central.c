#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define MAX_C 20
typedef struct { int fd; char nom[32]; int type; } Client;
Client cls[MAX_C];

void diffuse(char *m) {
    for(int i=0; i<MAX_C; i++) if(cls[i].fd != -1 && cls[i].type == 3) send(cls[i].fd, m, strlen(m), 0);
}

int main(int argc, char **argv) {
    int master = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {AF_INET, htons(atoi(argv[1])), INADDR_ANY};
    int opt=1; setsockopt(master, SOL_SOCKET, SO_REUSEADDR, &opt, 4);
    bind(master, (struct sockaddr*)&a, sizeof(a));
    listen(master, 5);

    int udp = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in u_in = {AF_INET, htons(9090), INADDR_ANY};
    bind(udp, (struct sockaddr*)&u_in, sizeof(u_in));

    struct sockaddr_in u_rmi = {AF_INET, htons(9091), inet_addr("127.0.0.1")};

    for(int i=0; i<MAX_C; i++) cls[i].fd = -1;

    while(1) {
        fd_set fds; FD_ZERO(&fds); FD_SET(master, &fds); FD_SET(udp, &fds);
        int m = (master > udp) ? master : udp;
        for(int i=0; i<MAX_C; i++) { if(cls[i].fd!=-1) { FD_SET(cls[i].fd, &fds); if(cls[i].fd>m) m=cls[i].fd; }}
        
        select(m+1, &fds, NULL, NULL, NULL);

        if (FD_ISSET(master, &fds)) {
            int n = accept(master, 0, 0);
            for(int i=0; i<MAX_C; i++) if(cls[i].fd==-1) { cls[i].fd=n; break; }
        }

        if (FD_ISSET(udp, &fds)) {
            char b[256]; int n = recv(udp, b, 255, 0); b[n]='\0';
            char p[32]; sscanf(b, "%*s %s", p);
            for(int i=0; i<MAX_C; i++) if(cls[i].fd!=-1 && cls[i].type==2 && strcmp(cls[i].nom,p)==0) send(cls[i].fd, b, n, 0);
        }

        for(int i=0; i<MAX_C; i++) {
            if (cls[i].fd != -1 && FD_ISSET(cls[i].fd, &fds)) {
                char b[256]; int n = read(cls[i].fd, b, 255);
                if (n <= 0) { close(cls[i].fd); cls[i].fd=-1; continue; }
                b[n]='\0';
                if (strncmp(b, "THERMO", 6)==0) { cls[i].type=1; sscanf(b, "THERMO %s", cls[i].nom); }
                else if (strncmp(b, "CHAUFFAGE", 9)==0) { cls[i].type=2; sscanf(b, "CHAUFFAGE %s", cls[i].nom); }
                else if (strncmp(b, "CONSOLE", 7)==0) { cls[i].type=3; }
                else if (strncmp(b, "TEMP", 4)==0 || strncmp(b, "STAT", 4)==0) {
                    char relay[256];
                    if(b[0]=='T') { char p[32]; int v; sscanf(b, "TEMP %s %d", p, &v); sprintf(relay, "UPDATE %s %d\n", p, v); }
                    else strcpy(relay, b);
                    diffuse(relay);
                    sendto(udp, relay, strlen(relay), 0, (struct sockaddr*)&u_rmi, sizeof(u_rmi));
                }
            }
        }
    }
}