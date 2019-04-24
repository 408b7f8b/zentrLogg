#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    struct addrinfo hints;
    struct addrinfo* res;
    int sock;
} zentrLoggStruktur;

void zentrLogg_InitStruktur(zentrLoggStruktur* Struktur, char* service_address, char* service_port){
    memset(&(Struktur->hints),0,sizeof(struct addrinfo));
    Struktur->hints.ai_family=AF_UNSPEC;
    Struktur->hints.ai_socktype=SOCK_DGRAM;
    Struktur->hints.ai_protocol=0;
    Struktur->hints.ai_flags=AI_ADDRCONFIG;

    Struktur->res = 0;

    int err = getaddrinfo(service_address, service_port, &(Struktur->hints), &(Struktur->res));
    if (err != 0) {
        printf("failed to resolve remote socket address (err=%d)",err);
        exit(1);
    }

    Struktur->sock = socket(Struktur->res->ai_family, Struktur->res->ai_socktype, Struktur->res->ai_protocol);
    if (Struktur->sock ==-1) {
        printf("%s",strerror(errno));
        exit(1);
    }
}

ssize_t zentrLogg_Send(zentrLoggStruktur* Struktur, char* nachricht){
    ssize_t r = sendto(Struktur->sock, nachricht, strlen(nachricht), 0, Struktur->res->ai_addr, Struktur->res->ai_addrlen);
    return r;
}

bool zentrLogg_Send_Boolean(zentrLoggStruktur* Struktur, char* nachricht){
    return (zentrLogg_Send(Struktur, nachricht) != -1);
}

int main(){

    zentrLoggStruktur Struktur;

    zentrLogg_InitStruktur(&Struktur, "127.0.0.1", "1338");

    bool b = zentrLogg_Send_Boolean(&Struktur, "START");
    b = zentrLogg_Send_Boolean(&Struktur, "testnachricht");
    b = zentrLogg_Send_Boolean(&Struktur, "HALT");

    return b;
}

