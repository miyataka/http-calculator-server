#include <sys/types.h>

enum http_method {
    HTTP_METHOD_UNKNOWN,
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    // TODO later
};

enum http_version {
    HTTP_VERSION_UNKNOWN,
    HTTP_VERSION_0_9,
    HTTP_VERSION_1_0,
    HTTP_VERSION_1_1,
    // TODO later
};

struct str_slice {
  const char* ptr;
  size_t len;
};

struct http_header_field {
  struct str_slice name;
  struct str_slice value;
};

struct http_request {
    enum http_method method;
    struct str_slice target;
    enum http_version version;

    struct str_slice path;
    struct str_slice query;

    struct str_slice header_lines[32]; // raw line
    size_t header_line_count;

    struct http_header_field headers[32]; // parsed field
    size_t header_count;
};

ssize_t recv_http_header(int client_fd, char* buffer, size_t buffer_size);
ssize_t response_fixed(int client_fd);
int parse_http_request_head(char* buf, struct http_request* req);
struct http_header_field* get_header(struct http_request* req, char* name);
int slice_to_size_t(struct str_slice s, size_t* out);
