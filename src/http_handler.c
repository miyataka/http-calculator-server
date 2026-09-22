#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "http.h"
#include "tcp_server.h"

char* CALC_FIXED_RESPONSE = "HTTP/1.1 200 OK\r\n"
                            "Content-Length: 9\r\n"
                            "Connection: close\r\n"
                            "\r\n"
                            "calclated\r\n";

ssize_t calc_handler(int socket, struct http_request req) {
    ssize_t sum_sent = send_n(socket, CALC_FIXED_RESPONSE, strlen(CALC_FIXED_RESPONSE));
    if (sum_sent == -1) {
        perror("send_n");
        return -1;
    }
    return sum_sent;
}


char* NOT_FOUND_RESPONSE = "HTTP/1.1 404 Not Found\r\n"
                            "Content-Length: 9\r\n"
                            "Connection: close\r\n"
                            "\r\n"
                            "Not Found\r\n";

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
                            "Bad Request\r\n";


ssize_t bad_request(int socket) {
    ssize_t sum_sent = send_n(socket, BAD_REQUEST_RESPONSE, strlen(BAD_REQUEST_RESPONSE));
    if (sum_sent == -1) {
        perror("send_n");
        return -1;
    }
    return sum_sent;
}
