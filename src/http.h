#include <sys/types.h>

enum http_method {
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_UNKNOWN, // TODO later
};

enum http_version {
    HTTP_VERSION_0_9,
    HTTP_VERSION_1_0,
    HTTP_VERSION_1_1,
    HTTP_VERSION_UNKNOWN, // TODO later
};

struct str_slice {
  const char* ptr;
  size_t len;
};

struct http_header {
    enum http_method method;
    char* target;
    size_t target_len;
    enum http_version version;
    // TODO other headers
};

ssize_t recv_http_header(int client_fd, char* buffer, size_t buffer_size);
ssize_t response_fixed(int client_fd);
void parse_http_header(char* buf, struct http_header* header);
