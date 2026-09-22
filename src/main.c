#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "tcp_server.h"
#include "http.h"
#include "http_handler.h"

int main() {
    int sock = create_tcp_server();
    if (sock == -1) {
        printf("create_tcp_server error: %s\n", strerror(errno));
        return -1;
    }

    if (listen(sock, 100) == -1) {
        perror("listen error");
        return -1;
    }

    for (;;) {
        int client_fd = accept(sock, NULL, NULL);
        if (client_fd == -1) {
            perror("accept error");
            return -1;
        }

        // recv loop
        char buf[1024] = {0};
        // TODO handle if it received over 1kB
        // ssize_t sum_received = recv_until_eof(client_fd, buf, sizeof(buf));
        ssize_t sum_received = recv_http_header(client_fd, buf, sizeof(buf));
        printf("%d byte\n%s\n", (int)sum_received, buf);
        struct http_request req = {0};
        struct str_slice t = {0};
        req.target = t;
        if (parse_http_header(buf, &req) == -1) {
            bad_request(client_fd);
        }

        printf("http_method: %d\n", req.method);
        printf("http_target: %p\n", req.target.ptr);
        printf("http_target_len: %d\n", (int)req.target.len);
        printf("http_version: %d\n", req.version);

        // routing
        char target[req.target.len+1];
        memcpy(target, req.target.ptr, req.target.len);
        target[req.target.len] = '\0';

        ssize_t sum_sent = 0;
        if (req.method == HTTP_METHOD_GET && memcmp(target, "/calc", 5) == 0) {
            sum_sent = calc_handler(client_fd, req);
            if (sum_sent == -1) {
                perror("calc_handler");
            }
        } else if (req.method == HTTP_METHOD_GET && strlen(target) == 1 && memcmp(target, "/", 1) == 0) {
            sum_sent = response_fixed(client_fd);
            if (sum_sent == -1) {
                perror("response_fixed");
            }
        } else {
            sum_sent = not_found(client_fd);
            if (sum_sent == -1) {
                perror("not_found");
            }
        }

        if (close(client_fd) == -1) {
            perror("close error");
            return -1;
        }
    }

    if (close(sock) == -1) {
        perror("close error");
        return -1;
    }
    return 0;
}
