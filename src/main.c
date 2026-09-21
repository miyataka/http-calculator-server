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
        char buf[1024];
        buf[0] = '\0';
        // TODO handle if it received over 1kB
        // ssize_t sum_received = recv_until_eof(client_fd, buf, sizeof(buf));
        ssize_t sum_received = recv_http_header(client_fd, buf, sizeof(buf));
        printf("%d byte\n%s\n", (int)sum_received, buf);

        // send loop
        ssize_t sum_sent = response_fixed(client_fd);
        if (sum_sent == -1) {
            perror("send_n");
            return -1;
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
