#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#include "tcp_server.h"


char* FIXED_RESPONSE = "HTTP/1.1 200 OK\n"
                       "Content-Length: 5\n"
                       "Connection: close\n"
                       "\r\n"
                       "hello\n";


ssize_t response_fixed(int client_fd) {
    ssize_t sum_sent = send_n(client_fd, FIXED_RESPONSE, strlen(FIXED_RESPONSE));
    if (sum_sent == -1) {
        perror("send_n");
        return -1;
    }
    return sum_sent;
}
