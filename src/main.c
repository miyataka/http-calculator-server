#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

int main() {
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    // TODO error handle

    int option_on = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                &option_on, sizeof(option_on)) == -1) {
        printf("setsockopt error");
    }
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr.s_addr = INADDR_ANY,
    };

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind error");
    }

    // listen
    if (listen(sock, 100) == -1) {
        perror("listen error");
    }

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

    close(sock);
    // TODO error handle
    return 0;
}
