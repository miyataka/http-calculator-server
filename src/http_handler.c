#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

#include "http.h"
#include "tcp_server.h"
#include "http_handler.h"

char* NOT_FOUND_RESPONSE = "HTTP/1.1 404 Not Found\r\n"
                            "Content-Length: 9\r\n"
                            "Connection: close\r\n"
                            "\r\n"
                            "Not Found";

ssize_t not_found(int socket) {
    char* content = "Not Found";
    ssize_t sum_sent = send_response(socket, HTTP_STATUS_NOT_FOUND, content, strlen(content));
    if (sum_sent == -1) {
        perror("send_response");
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
    char* content = "Bad Request";
    ssize_t sum_sent = send_response(socket, HTTP_STATUS_BAD_REQUEST, content, strlen(content));
    if (sum_sent == -1) {
        perror("send_response");
        return -1;
    }
    return sum_sent;
}

char* CALC_FIXED_RESPONSE = "HTTP/1.1 200 OK\r\n"
                            "Content-Length: 9\r\n"
                            "Connection: close\r\n"
                            "\r\n"
                            "calclated";

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

int parse_calc_expr(struct str_slice s, struct calc_expr* out) {
    size_t pos_op;
    struct str_slice ss = strip(s);
    if (find_operator(ss, &pos_op) == -1) {
        return -1;
    }

    struct str_slice l = { .ptr = ss.ptr,          .len = pos_op };
    struct str_slice r = { .ptr = ss.ptr+pos_op+1, .len = ss.len - pos_op - 1 };

    long lhs, rhs;
    if (slice_to_long(strip(l), &lhs) == -1) {
        return -1;
    }
    if (slice_to_long(strip(r), &rhs) == -1) {
        return -1;
    }

    struct calc_expr ce = {
        .lhs = lhs,
        .op = ss.ptr[pos_op],
        .rhs = rhs,
    };
    *out = ce;
    return 0;
}

int eval_calc_expr(const struct calc_expr* e, long* out) {
    long r = 0;
    switch (e->op) {
        case '+':
            if (__builtin_add_overflow(e->lhs, e->rhs, &r)) {
                return -1;
            }
            break;
        case '-':
            if (__builtin_sub_overflow(e->lhs, e->rhs, &r)) {
                return -1;
            }
            break;
        case '*':
            if (__builtin_mul_overflow(e->lhs, e->rhs, &r)) {
                return -1;
            }
            break;
        case '/':
            if (e->rhs == 0) {
                return -1;
            }
            if (e->lhs == LONG_MIN && e->rhs == -1) {
                return -1;
            }
            r = e->lhs / e->rhs;
            break;
        default:
            return -1;
    }
    *out = r;
    return 0;
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
