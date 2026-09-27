#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "http.h"
#include "tcp_server.h"

char* NOT_FOUND_RESPONSE = "HTTP/1.1 404 Not Found\r\n"
                            "Content-Length: 9\r\n"
                            "Connection: close\r\n"
                            "\r\n"
                            "Not Found";

ssize_t not_found(int socket) {
    ssize_t sum_sent = send_n(socket, NOT_FOUND_RESPONSE, strlen(NOT_FOUND_RESPONSE));
    if (sum_sent == -1) {
        perror("send_n");
        return -1;
    }
    return sum_sent;
}

char* BAD_REQUEST_RESPONSE = "HTTP/1.1 400 Bad Request\r\n"
                            "Content-Length: 11\r\n"
                            "Connection: close\r\n"
                            "\r\n"
                            "Bad Request";


ssize_t bad_request(int socket) {
    ssize_t sum_sent = send_n(socket, BAD_REQUEST_RESPONSE, strlen(BAD_REQUEST_RESPONSE));
    if (sum_sent == -1) {
        perror("send_n");
        return -1;
    }
    return sum_sent;
}

char* CALC_FIXED_RESPONSE = "HTTP/1.1 200 OK\r\n"
                            "Content-Length: 9\r\n"
                            "Connection: close\r\n"
                            "\r\n"
                            "calclated";

struct calc_expr {
    long lhs;
    char op;
    long rhs;
};

int find_operator(struct str_slice s, size_t* pos) {
    // `i` starts from 1, because it maybe lhs's sign when i=0
    for (int i = 1; i < s.len; i++) {
        if (s.ptr[i] == '+' ||
            s.ptr[i] == '-' ||
            s.ptr[i] == '*' ||
            s.ptr[i] == '/') {
            *pos=i;
            return 0;
        }
    }
    return -1;
}

ssize_t calc_handler(int socket, struct http_request req) {
    if (req.param_count == 0) {
        return bad_request(socket);
    }
    struct http_query_param* q_param = get_query_param(&req, "q");
    if (q_param == NULL) {
        return bad_request(socket);
    }
    printf("q: %.*s\n", (int)q_param->value.len, q_param->value.ptr);

    struct http_response res = create_http_response(200);

    ssize_t sum_sent = send_n(socket, q_param->value.ptr, q_param->value.len);
    if (sum_sent == -1) {
        perror("send_n");
        return -1;
    }
    return sum_sent;
}
