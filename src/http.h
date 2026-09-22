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

struct http_request {
    enum http_method method;
    struct str_slice target;
    enum http_version version;

    char* host;
    size_t host_len;

    char* content_length;
    size_t content_length_len;

    // TODO other headers
};

ssize_t recv_http_header(int client_fd, char* buffer, size_t buffer_size);
ssize_t response_fixed(int client_fd);
void parse_http_header(char* buf, struct http_request* req);
