#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "tcp_server.h"

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
        ssize_t num_received = -1;
        ssize_t sum_received = 0;
        while(num_received != 0) {
            num_received = recv(client_fd,
                                buf + sum_received,
                                sizeof(buf) - sum_received,
                                0);
            if (num_received == -1) {
                perror("recv error");
                return -1;
            }
            sum_received += num_received;
            printf("%d byte received\n", (int)sum_received);
        }
        // handle if it received over 1kB

        // send loop
        ssize_t num_sent = 0;
        while(num_sent != sum_received) {
            ssize_t sent = 0;
            sent = send(client_fd,
                    buf + num_sent,
                    sum_received - num_sent,
                    0);
            if (sent == -1) {
                perror("send error");
                return -1;
            }
            num_sent += sent;
            printf("%d byte sent\n", (int)num_sent);
        }

        close(client_fd);
        // TODO error handle
    }

    close(sock);
    // TODO error handle
    return 0;
}
