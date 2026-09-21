#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "tcp_server.h"


char* FIXED_RESPONSE = "HTTP/1.1 200 OK\r\n"
                       "Content-Length: 5\r\n"
                       "Connection: close\r\n"
                       "\r\n"
                       "hello\r\n\0";

char* END_OF_HEADER = "\r\n\r\n\0";

ssize_t recv_http_header(int socket, char* buffer, size_t buffer_size) {
    ssize_t sum_received = 0;

    while((size_t)sum_received < buffer_size - 1) {
        ssize_t num_received = recv(socket,
                                    buffer + sum_received,
                                    buffer_size - 1 - sum_received,
                                    0);
        if (num_received == -1) {
            perror("recv error");
            return -1;
        }
        if (num_received == 0) break;
        sum_received += num_received;
        buffer[sum_received] = '\0';
        printf("%d byte received. total %d bytes\n", (int)num_received, (int)sum_received);
        if (strstr(buffer, END_OF_HEADER) != NULL) break;
    }

    return sum_received;
}

ssize_t response_fixed(int client_fd) {
    ssize_t sum_sent = send_n(client_fd, FIXED_RESPONSE, strlen(FIXED_RESPONSE));
    if (sum_sent == -1) {
        perror("send_n");
        return -1;
    }
    return sum_sent;
}
