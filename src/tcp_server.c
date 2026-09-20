#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>

int create_tcp_server() {
    int sock = socket(PF_INET6, SOCK_STREAM, 0);
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

    int option_off = 0;
    if(setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &option_off, sizeof(option_off)) == -1) {
        perror("setsockopt error");
        close(sock);
        return -1;
    }

    struct sockaddr_in6 addr = {
        .sin6_family = AF_INET6,
        .sin6_port = htons(8080),
        .sin6_addr = in6addr_any,
    };
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind error");
        close(sock);
        return -1;
    }
    return sock;
}

ssize_t recv_until_eof(int socket, void *buffer, size_t buffer_size) {
    ssize_t num_received = -1;
    ssize_t sum_received = 0;

    while(num_received != 0) {
        num_received = recv(socket,
                            buffer + sum_received,
                            buffer_size - sum_received,
                            0);
        if (num_received == -1) {
            perror("recv error");
            return -1;
        }
        sum_received += num_received;
        printf("%d byte received. total %d bytes\n", (int)num_received, (int)sum_received);
    }

    return sum_received;
}


ssize_t send_n(int socket, void *buffer, size_t buffer_size) {
    ssize_t num_sent = 0;
    while(num_sent != (ssize_t)buffer_size) {
        ssize_t sent = 0;
        sent = send(socket,
                    buffer + num_sent,
                    buffer_size - num_sent,
                    0);
        if (sent == -1) {
            perror("send error");
            return -1;
        }
        num_sent += sent;
        printf("%d byte sent\n", (int)num_sent);
    }
    return num_sent;
}
