#include <sys/types.h>

ssize_t recv_http_header(int client_fd, char* buffer, size_t buffer_size);
ssize_t response_fixed(int client_fd);
