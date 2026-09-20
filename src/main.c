#include <stdio.h>
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

        char buf[1024];
        int num_received = -1;
        int sum_received = 0;
        while(num_received != 0) {
            num_received = recv(client_fd,
                                buf + sum_received,
                                sizeof(buf) - sum_received,
                                0);
            sum_received += num_received;
            printf("%d byte received\n", sum_received);
        }
        // handle if it received over 1kB

        send(client_fd, buf, sum_received, 0);
        // TODO error handle

        close(client_fd);
        // TODO error handle
    }

    close(sock);
    // TODO error handle
    return 0;
}
