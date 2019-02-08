#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

int main(){
    const char* hostname=0;
    struct addrinfo hints;
    memset(&hints,0,sizeof(hints));
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_protocol=0;
    hints.ai_flags=AI_ADDRCONFIG;
    struct addrinfo* res=0;
    int err=getaddrinfo(hostname,"1338",&hints,&res);
    if (err!=0) {
        printf("failed to resolve remote socket address (err=%d)",err);
        exit(1);
    }

    int fd=socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    if (fd==-1) {
        printf("%s",strerror(errno));
        exit(1);
    }

    char* test = "testnachricht";

    if (sendto(fd,test,strlen(test),0,
               res->ai_addr,res->ai_addrlen)==-1) {
        printf("%s",strerror(errno));
        exit(1);
    }
}

