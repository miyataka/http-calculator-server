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

char* find_space(char* buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (buf[i] == ' ') {
            return &buf[i];
        }
    }
    return NULL;
}


enum http_method decide_http_method(struct str_slice method) {
    if (method.len <= 0) return HTTP_METHOD_UNKNOWN;

    switch (method.ptr[0]) {
        case 'G':
            return HTTP_METHOD_GET;
        case 'P':
            return HTTP_METHOD_POST;
        default:
            return HTTP_METHOD_UNKNOWN;
    }
}

enum http_version decide_http_version(struct str_slice version) {
    if (version.len <= 7) return HTTP_VERSION_UNKNOWN;

    switch (version.ptr[5]) {
        case '0':
            return HTTP_VERSION_0_9;
        case '1':
            switch (version.ptr[7]) {
                case '0':
                    return HTTP_VERSION_1_0;
                case '1':
                    return HTTP_VERSION_1_1;
            }
        default:
            return HTTP_VERSION_UNKNOWN;
    }
}

int parse_request_line(struct http_request* req) {
    if (req->header_line_count == 0) return -1;
    struct str_slice request_line = req->header_lines[0];

    const char* start = request_line.ptr;
    char* end = request_line.ptr + request_line.len;

    char* pos_space = find_space(start, end - start);
    if (pos_space == NULL) return -1;
    struct str_slice method = {
        .ptr = start,
        .len = pos_space - start,
    };

    start = pos_space+1;
    pos_space = find_space(pos_space+1, end - start);
    if (pos_space == NULL) return -1;
    struct str_slice target = {
        .ptr = start,
        .len = pos_space - start,
    };

    start = pos_space+1;
    struct str_slice version = {
        .ptr = start,
        .len = end - start,
    };

    req->method = decide_http_method(method);
    req->target = target;
    req->version = decide_http_version(version);

    return 0;
}

int parse_http_request_head(char* buf, struct http_request* req) {
    char* end_of_header = strstr(buf, "\r\n\r\n");
    if (end_of_header == NULL) return -1;

    int i = 0;
    char* next_line = buf; // requestの先頭から
    while (i < 32 && next_line < end_of_header) {
        char* line_tail = strstr(next_line, "\r\n");
        if (line_tail == NULL) break;

        struct str_slice line;
        line.ptr = next_line;
        line.len = line_tail - next_line;

        req->header_lines[i] = line;
        next_line = line_tail + 2;
        i++;
    }
    req->header_line_count = i;

    if (parse_request_line(req) == -1) {
        return -1;
    }

    return 0;
}
