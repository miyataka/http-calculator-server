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

    // listen
    if (listen(sock, 100) == -1) {
        perror("listen error");
    }

    for (;;) {
        // accept
        int client_fd = accept(sock, NULL, NULL);

        // recv
        char buf[1024];
        int num_received = recv(client_fd, buf, sizeof(buf), 0);
        printf("%d byte received\n", num_received);
        // TODO error handle

        // send
        send(client_fd, buf, num_received, 0);
        // TODO error handle

        // close
        close(client_fd);
        // TODO error handle
    }

    close(sock);
    // TODO error handle
    return 0;
}
