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
#include <stdbool.h>

bool run;
char* file_name;
FILE* file_handler;
char* port;
long rcv_timeout_usec;
bool flag_usec_in_timestamp;

void getDateTime(char buffer[], int flag_usec) {
    struct tm* tm_info;
    struct timeval tv;
    gettimeofday(&tv, NULL);

    tm_info = localtime(&tv.tv_sec);
    strftime(buffer, 26, "%Y-%m-%dT%H:%M:%S", tm_info);

    if (flag_usec_in_timestamp) {
        sprintf(buffer, "%s.%06li", buffer, tv.tv_usec);
    } else {
        int millisec = 0;
        millisec = (int) tv.tv_usec / 1000;
        sprintf(buffer, "%s.%03i", buffer, millisec);
    }
}

void sigint(int r) {
    run = false;
}

int starteDatei() {
    int i;
    file_name = calloc(35, sizeof(char*));

    getDateTime(file_name, 0);
    strcat(file_name, ".log");

    if (file_handler != NULL) {
        fclose(file_handler);
    }

    file_handler = fopen(file_name, "a");

    if (file_handler == NULL) {
        printf("Error opening file!\n");
        return -1;
    }

    return 0;
}

void handle_datagram(char* s, char b[], size_t l) {

    char t_b[34] = {0};
    getDateTime(t_b, flag_usec_in_timestamp);

    char b_l[l + 1];
    b_l[l] = 0;
    strncpy(b_l, b, l);

    fprintf(file_handler, "%s: %s: %s\n", s, t_b, b_l);

    printf("%s: Got and wrote datagram from %s\n", t_b, s);
}

int main(int argc, char** argv) {

    signal(SIGINT, sigint);
    signal(SIGTERM, sigint);

    run = true;
    file_handler = NULL;
    port = "1338";
    rcv_timeout_usec = 100000;
    flag_usec_in_timestamp = false;

    int i;
    for (i = 0; i < argc - 1; ++i) {
        if (strncmp(argv[i], "-p", 2) == 0) {
            port = argv[++i];
        }
        else if (strncmp(argv[i], "-u", 2) == 0) {
            rcv_timeout_usec = strtoul(argv[++i], NULL, 10);
        }
        else if (strncmp(argv[i], "-z", 2) == 0) {
            flag_usec_in_timestamp = true;
        }
    }

    const char* hostname = 0;
    struct addrinfo* hints = (struct addrinfo*) calloc(1, sizeof(struct addrinfo));

    hints->ai_family = AF_UNSPEC;
    hints->ai_socktype = SOCK_DGRAM;
    hints->ai_protocol = 0;
    hints->ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    struct addrinfo* res = 0;
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
    read_timeout.tv_usec = rcv_timeout_usec;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));

    struct sockaddr_storage src_addr;
    socklen_t src_addr_len = sizeof(src_addr);

    bool mitschreiben = 0;
    starteDatei();

    while (run) {
        char buffer[1024] = {0};

        ssize_t count = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr*) &src_addr, &src_addr_len);
        if (count == -1 && errno != EAGAIN) {
            char t_b[34] = {0};
            getDateTime(t_b, 1);
            printf("%s: %s\n", t_b, strerror(errno));
        } else if (count == sizeof(buffer)) {
            printf("datagram too large for buffer: truncated");
        } else {
            if (mitschreiben) {
                if (strncmp(buffer, "HALT", 5) == 0) {
                    mitschreiben = 0;
                } else {
                    char* ipString = inet_ntoa(((struct sockaddr_in*) &src_addr)->sin_addr);
                    handle_datagram(ipString, buffer, (size_t) count);
                }
            } else {
                if (strncmp(buffer, "START", 5) == 0) {
                    mitschreiben = 1;
                }
            }
        }
    }

    fclose(file_handler);

    return 0;

}