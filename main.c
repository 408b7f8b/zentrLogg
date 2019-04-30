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

//size-defines
#define PATH_LENGTHS 256
#define MAX_MSG_LENGTH 1024

//global variables
//configuration
char port[6] = "1338";
char path[PATH_LENGTHS] = {0};
long rcv_timeout_usec = 100000;
bool flag_usec_in_timestamp = false;
int cmd_mode = 2;

//internal
bool run = true;
char file_name[PATH_LENGTHS] = {0};
FILE* file_handler = NULL;
int fd;
struct sockaddr_storage src_addr;
socklen_t src_addr_len = sizeof(src_addr);

//gibe timestamp
void getDateTime(char buffer[], bool flag_usec) {
    struct tm* tm_info;
    struct timeval tv;
    gettimeofday(&tv, NULL);

    tm_info = localtime(&tv.tv_sec);
    strftime(buffer, 26, "%Y-%m-%dT%H:%M:%S", tm_info);

    if (flag_usec_in_timestamp) {
        sprintf(buffer, "%s.%06li", buffer, tv.tv_usec);
    } else {
        int millisecs = 0;
        millisecs = (int) tv.tv_usec / 1000;
        sprintf(buffer, "%s.%03i", buffer, millisecs);
    }
}

//file handling
void startFile(char* path_prefix) {
    char timestamp[PATH_LENGTHS / 2] = {0};
    getDateTime(timestamp, 0);
    strcat(timestamp, ".log");

    for (int i = 0; i < sizeof(file_name); ++i) {
        file_name[i] = 0;
    }

    strncpy(file_name, path_prefix, strlen(path_prefix));
    strncat(file_name, timestamp, strlen(timestamp));
}

//message handling
void handle_datagram(char* s, char b[], size_t l) {
    char t_b[34] = {0};
    getDateTime(t_b, flag_usec_in_timestamp);

    char b_l[l + 1];
    b_l[l] = 0;
    strncpy(b_l, b, l);

    if (file_handler != NULL) {
        fclose(file_handler);
    }

    file_handler = fopen(file_name, "a");

    if (file_handler == NULL) {
        printf("%s: Got datagram from %s, but failed to open file\n", t_b, s);
    } else {
        fprintf(file_handler, "%s: %s: %s\n", s, t_b, b_l);
        printf("%s: Got and wrote datagram from %s\n", t_b, s);
        fclose(file_handler);
        file_handler = NULL;
    }
}

//macros
#define cmdlineparamvar(param, cmp_string)                      \
    if(strcmp(param, cmp_string) == 0 && strlen(param) == strlen(cmp_string))

#define while_std                                               \
    while (run) {                                               \
    char buffer[MAX_MSG_LENGTH] = {0};                          \
                                                                \
    ssize_t rcvd_bytes = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr*) &src_addr, &src_addr_len);\
                                                                \
    switch(rcvd_bytes){                                         \
        case -1:{                                               \
            if (errno != EAGAIN) {                              \
                char t_b[34] = {0};                             \
                getDateTime(t_b, 1);                            \
                printf("%s: %s\n", t_b, strerror(errno));       \
            }                                                   \
            break;                                              \
        }                                                       \
        case sizeof(buffer):{                                   \
            printf("datagram too large for buffer: ignored");   \
            break;                                              \
        }                                                       \
        default:{

#define close_while_std                                         \
    }}}

#define handle_msg                                              \
    char* ipString = inet_ntoa(((struct sockaddr_in*) &src_addr)->sin_addr);\
    handle_datagram(ipString, buffer, (size_t) rcvd_bytes);

void while_cmd() {
    while_std
                switch (cmd_mode) {
                    case 0: {
                        if (strncmp(buffer, "START", 5) == 0) {
                            cmd_mode = 1;
                        }
                        break;
                    }
                    case 1: {
                        if (strncmp(buffer, "HALT", 5) == 0) {
                            cmd_mode = 0;
                        } else {
                            handle_msg
                        }
                        break;
                    }
                    default: {
                    }
                }
    close_while_std
}

void while_non_cmd() {
    while_std
                handle_msg
    close_while_std
}

void sigint(int r) {
    run = false;
}

int main(int argc, char** argv) {

    signal(SIGINT, sigint);
    signal(SIGTERM, sigint);
    signal(SIGABRT, sigint);

    if(argc == 2){
        cmdlineparamvar(argv[1], "--help"){
            printf("Zentrales Loggen über UDP\n(c)2019 D. A. Breunig\n\n");
            printf("Parameter   Explanation                       Preset value     Example\n");
            printf("-p          Define a port for listening.      1338             -p 8100\n");
            printf("-u          Define rcv-timeout in microsecs.  100000           -u 10000\n");
            printf("-z          Include microsecs in timestamps.  no               -z\n");
            printf("-c          Enable Start/Halt over UDP.       no               -c\n");
            printf("-o          Prefix for logfile path           none             -o /home/pi/log/\n");
        }

        return 0;
    }

    int i;
    for (i = 0; i < argc; ++i) {
        cmdlineparamvar(argv[i], "-p") {
            if(i == argc-1){
                printf("Value for -p is missing. Exiting.\n");
                return -1;
            }

            char tmp[sizeof(port)] = {0};
            strncpy(port, tmp, sizeof(port));
            strncpy(port, argv[++i], sizeof(port));
        } else cmdlineparamvar(argv[i], "-u") {
            if(i == argc-1){
                printf("Value for -u argument is missing. Exiting.\n");
                return -1;
            }

            rcv_timeout_usec = strtoul(argv[++i], NULL, 10);
        } else cmdlineparamvar(argv[i], "-z") {
            flag_usec_in_timestamp = true;
        } else cmdlineparamvar(argv[i], "-c") {
            cmd_mode = 0;
        } else cmdlineparamvar(argv[i], "-o") {
            if(i == argc-1){
                printf("Value for -o is missing. Exiting.\n");
                return -1;
            }

            if (strlen(argv[++i]) > PATH_LENGTHS) {
                printf("-o path too long\n");
                return -1;
            }

            strncpy(path, argv[i], strlen(argv[i]));
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
    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
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

    startFile(path);

    if (cmd_mode == 2) {
        while_cmd();
    } else {
        while_non_cmd();
    }

    free(hints);

    return 0;

}