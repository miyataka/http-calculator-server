#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>

int create_tcp_server() {
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket error");
        return -1;
    }

    int option_on = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &option_on, sizeof(option_on)) == -1) {
        perror("setsockopt error");
        close(sock);
        return -1;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr.s_addr = INADDR_ANY,
    };
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind error");
        close(sock);
        return -1;
    }
    return sock;
}
