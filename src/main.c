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

    // send
    const char *msg = "hello\n";
    send(client_fd, msg, strlen(msg), 0);
    // TODO error handle

    // close
    close(client_fd);
    // TODO error handle

    close(sock);
    // TODO error handle
    return 0;
}
