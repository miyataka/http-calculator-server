#include <sys/types.h>

int create_tcp_server();
ssize_t recv_until_eof(int socket, char *buffer, size_t buffer_size);
ssize_t send_n(int socket, char *buffer, size_t buffer_size);
