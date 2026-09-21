#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "tcp_server.h"
#include "http.h"

char* FIXED_RESPONSE = "HTTP/1.1 200 OK\r\n"
                       "Content-Length: 5\r\n"
                       "Connection: close\r\n"
                       "\r\n"
                       "hello\r\n";

char* END_OF_HEADER = "\r\n\r\n";

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

char* find_crlf(char* buf, size_t len) {
    for (size_t i = 0; i + 1 < len; i++) {
        if (buf[i] == '\r' && buf[i+1] == '\n') {
            return &buf[i];
        }
    }
    return NULL;
}

char* find_space(char* buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (buf[i] == ' ') {
            return &buf[i];
        }
    }
    return NULL;
}

enum http_method parse_http_method(char* buf, size_t len) {
    char* space = find_space(buf, len);
    if (space == NULL) return HTTP_METHOD_UNKNOWN;
    size_t method_len = space - buf;
    if (method_len <= 0) return HTTP_METHOD_UNKNOWN;

    switch (buf[0]) {
        case 'G':
            return HTTP_METHOD_GET;
        case 'P':
            return HTTP_METHOD_POST;
        default:
            return HTTP_METHOD_UNKNOWN;
    }
}

void parse_http_target(char* buf, size_t len, struct http_header* header) {
    char* space = find_space(buf, len);
    size_t method_len = space - buf;
    space = find_space(buf+method_len+1, len-method_len-1);
    size_t target_len = space - (buf+method_len+1);

    if (target_len <= 0) return;
    header->target = buf + method_len + 1;
    header->target_len = target_len;
    return;
}

enum http_version parse_http_version(char* buf, size_t len) {
    char* space = find_space(buf, len);
    if (space == NULL) return HTTP_VERSION_UNKNOWN;
    size_t method_len = space - buf;
    if (method_len <= 0) return HTTP_VERSION_UNKNOWN;
    space = find_space(buf+method_len+1, len-method_len-1);
    char* version = space + 1;

    switch (version[5]) {
        case '0':
            return HTTP_VERSION_0_9;
        case '1':
            switch (version[7]) {
                case '0':
                    return HTTP_VERSION_1_0;
                case '1':
                    return HTTP_VERSION_1_1;
            }
        default:
            return HTTP_VERSION_UNKNOWN;
    }
}

void parse_http_header(char* buf, struct http_header* header) {
    size_t header_len = strlen(buf);
    char* end_of_line = find_crlf(buf, header_len);
    if (end_of_line == NULL) return;

    size_t len = end_of_line - buf;
    header->method = parse_http_method(buf, len);
    header->version = parse_http_version(buf, len);
    parse_http_target(buf, len, header);
}
