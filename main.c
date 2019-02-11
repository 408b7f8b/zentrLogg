#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <inttypes.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <arpa/inet.h>

int laufen;
char** fname;
FILE** f;
unsigned int f_l = 0;
char* port;
long usec;
int flag_usec_zeitstempel;
int flag_edateien;
int flag_start_halt;



void getDateTime(char buffer[], int flag_usec) {
    struct tm* tm_info;
    struct timeval tv;
    gettimeofday(&tv, NULL);

    tm_info = localtime(&tv.tv_sec);
    strftime(buffer, 26, "%Y-%m-%dT%H:%M:%S", tm_info);

    if(flag_usec_zeitstempel){
        sprintf(buffer,"%s.%06li", buffer, tv.tv_usec);
    }else{
        int millisec = 0;
        millisec = (int)tv.tv_usec/1000;
        sprintf(buffer,"%s.%03i", buffer, millisec);
    }
}

void sigint(int r) {
    laufen = 0;
}

int starteDatei(char* n){
    if(n == NULL){
        int i;
        fname = calloc(1, sizeof(char*));
        *fname = calloc(35, sizeof(char));

        getDateTime(fname[0], 0);
        strcat(fname[0], ".log");

        if(f != NULL){
            fclose(f[0]);
        }

        f = calloc(1, sizeof(FILE*));
        f[0] = fopen(fname[0], "a");

        if(*f == NULL){
            printf("Error opening file!\n");
            return -1;
        }

        f_l = 1;
    }else{
        unsigned int f_l_t = f_l +1;
        FILE** f_t = realloc(f, f_l_t*sizeof(FILE*));

        if(f_t == NULL){
            return -1;
        }

        char** fname_t = realloc(fname, f_l_t*sizeof(char*));

        if(fname_t == NULL){
            return -1;
        }

        fname_t[f_l] = calloc(1, (strlen(n)+5)*sizeof(char));

        if(fname_t[f_l] == NULL){
            return -1;
        }

        strcat(fname_t[f_l], n);
        strcat(fname_t[f_l], ".log");

        f[f_l] = fopen(fname[f_l], "a");

        if(f[f_l] == NULL){
            printf("Error opening file!\n");
            return -1;
        }

        f = f_t;
        ++f_l;
    }

    return 0;
}

void handle_datagram(char* s, char b[], size_t l) {
    char t_b[34] = {0};
    getDateTime(t_b, flag_usec_zeitstempel);

    char b_l[l+1];
    b_l[l] = 0;
    strncpy(b_l, b, l);

    if(flag_edateien){
        int i;

        for(i = 0; i < f_l; ++i){
            if(strncmp(fname[i], s, strlen(s)) == 0){
                fprintf(f[i], "%s: %s\n", t_b, b_l);
                return;
            }
        }
    }else{
        fprintf(f[0], "%s: %s: %s\n", s, t_b, b_l);
    }
}

int main(int argc, char **argv) {

    signal(SIGINT, sigint);
    signal(SIGTERM, sigint);

    laufen = 1;
    f = NULL;
    port = "1338";
    usec = 100000;
    flag_usec_zeitstempel = 0;

    int i;
    for(i = 0; i < argc-1; ++i){
        if(strncmp(argv[i], "-p", 2) == 0){
            port = argv[++i];
        }
        if(strncmp(argv[i], "-u", 2) == 0){
            usec = strtoul(argv[++i], NULL, 10);
        }
        if(strncmp(argv[i], "-z", 2) == 0){
            flag_usec_zeitstempel = 1;
        }
        if(strncmp(argv[i], "-e", 2) == 0){
            flag_edateien = 1;
        }
    }

    const char *hostname = 0;
    struct addrinfo *hints = (struct addrinfo *) calloc(1, sizeof(struct addrinfo));

    hints->ai_family = AF_UNSPEC;
    hints->ai_socktype = SOCK_DGRAM;
    hints->ai_protocol = 0;
    hints->ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    struct addrinfo *res = 0;
    int err = getaddrinfo(hostname, port, hints, &res);
    if (err != 0) {
        printf("failed to resolve local socket address (err=%d)", err);
        exit(1);
    }
    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1) {
        printf("%s", strerror(errno));
        exit(1);
    }
    if (bind(fd, res->ai_addr, res->ai_addrlen) == -1) {
        printf("%s", strerror(errno));
        exit(1);
    }
    freeaddrinfo(res);

    struct timeval read_timeout;
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = usec;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));

    char buffer[1024];
    struct sockaddr_storage src_addr;
    socklen_t src_addr_len = sizeof(src_addr);

    int betrieb = 0;

    if(!flag_edateien){
        starteDatei;
    }

    while (laufen) {
        ssize_t count = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *) &src_addr, &src_addr_len);
        if (count == -1 && errno != EAGAIN) {
            char t_b[34] = {0};
            getDateTime(t_b, 1);
            printf("%s: %s\n", t_b, strerror(errno));
        } else if (count == sizeof(buffer)) {
            printf("datagram too large for buffer: truncated");
        } else {
            if(betrieb){
                if(strncmp(buffer, "HALT", 5) == 0){
                    betrieb = 0;
                }else{
                    char* ipString = inet_ntoa(((struct sockaddr_in *) &src_addr)->sin_addr);
                    handle_datagram(ipString, buffer, (size_t)count);
                }
            }else{
                if(strncmp(buffer, "START", 5) == 0){
                    betrieb = 1;
                }
            }
        }
    }

    for(i = 0; i < f_l; ++i){
        fclose(f[i]);
    }

    return 0;

}