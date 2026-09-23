#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>
#include <stdlib.h>

#include "tcp_server.h"
#include "http.h"

char* FIXED_RESPONSE = "HTTP/1.1 200 OK\r\n"
                       "Content-Length: 5\r\n"
                       "Connection: close\r\n"
                       "\r\n"
                       "hello";

char* END_OF_HEADER = "\r\n\r\n";

ssize_t recv_http_header(int socket, char* buffer, size_t buffer_size) {
    ssize_t sum_received = 0;

    while((size_t)sum_received < buffer_size - 1) {
        ssize_t num_received = recv(socket,
                                    buffer + sum_received,
                                    buffer_size - 1 - sum_received,
                                    0);
        if (num_received == -1) {
            perror("recv error");
            return -1;
        }
        if (num_received == 0) break;
        sum_received += num_received;
        buffer[sum_received] = '\0';
        printf("%d byte received. total %d bytes\n", (int)num_received, (int)sum_received);
        if (strstr(buffer, END_OF_HEADER) != NULL) break;
    }

    return sum_received;
}

ssize_t response_fixed(int client_fd) {
    ssize_t sum_sent = send_n(client_fd, FIXED_RESPONSE, strlen(FIXED_RESPONSE));
    if (sum_sent == -1) {
        perror("send_n");
        return -1;
    }
    return sum_sent;
}

char* find_space(char* buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (buf[i] == ' ') {
            return &buf[i];
        }
    }
    return NULL;
}


enum http_method decide_http_method(struct str_slice method) {
    if (method.len <= 0) return HTTP_METHOD_UNKNOWN;

    switch (method.ptr[0]) {
        case 'G':
            return HTTP_METHOD_GET;
        case 'P':
            return HTTP_METHOD_POST;
        default:
            return HTTP_METHOD_UNKNOWN;
    }
}

enum http_version decide_http_version(struct str_slice version) {
    if (version.len <= 7) return HTTP_VERSION_UNKNOWN;

    switch (version.ptr[5]) {
        case '0':
            return HTTP_VERSION_0_9;
        case '1':
            switch (version.ptr[7]) {
                case '0':
                    return HTTP_VERSION_1_0;
                case '1':
                    return HTTP_VERSION_1_1;
            }
        default:
            return HTTP_VERSION_UNKNOWN;
    }
}

int parse_request_line(struct http_request* req) {
    if (req->header_line_count == 0) return -1;
    struct str_slice request_line = req->header_lines[0];

    const char* start = request_line.ptr;
    char* end = request_line.ptr + request_line.len;

    char* pos_space = find_space(start, end - start);
    if (pos_space == NULL) return -1;
    struct str_slice method = {
        .ptr = start,
        .len = pos_space - start,
    };

    start = pos_space+1;
    pos_space = find_space(pos_space+1, end - start);
    if (pos_space == NULL) return -1;
    struct str_slice target = {
        .ptr = start,
        .len = pos_space - start,
    };

    start = pos_space+1;
    struct str_slice version = {
        .ptr = start,
        .len = end - start,
    };

    req->method = decide_http_method(method);
    req->target = target;
    req->version = decide_http_version(version);

    return 0;
}

struct str_slice strip(struct str_slice slice) {
    if (slice.len == 0) return slice;

    const char* start = slice.ptr;
    const char* end = slice.ptr + slice.len;
    for (size_t i = 0; i < slice.len; i++) {
        if (slice.ptr[i] != ' ') {
            break;
        }
        start++;
    }
    while (end > start && end[-1] == ' ') end--;

    struct str_slice striped = {
        .ptr = start,
        .len = end - start,
    };
    return striped;
}

char* findchr(char* buf, size_t len, char c) {
    for (size_t i = 0; i < len; i++) {
        if (buf[i] == c) return &buf[i];
    }
    return NULL;
}

char* findstr(char* buf, size_t len, char* str) {
    size_t s_len = strlen(str);
    if (s_len == 0) return NULL;

    for (size_t i = 0; i + s_len <= len ; i++) {
        if (buf[i] != str[0]) continue;

        for (size_t j = 0; j < s_len; j++) {
            if (buf[i+j] != str[j]) {
                break;
            }
            if (j == s_len-1) return &buf[i];
        }
    }
    return NULL;
}

int parse_request_header_field(char* buf, size_t len, struct http_header_field* dst) {
    char* pos_end = findstr(buf, len, "\r\n");
    if (pos_end == NULL) {
        pos_end = buf + len;
    }
    char* colon = findchr(buf, pos_end-buf, ':');
    if (colon == NULL) return -1;

    struct str_slice name = {
        .ptr = buf,
        .len = colon - buf,
    };
    dst->name = name;

    struct str_slice v = {
        .ptr = colon + 1,
        .len = pos_end - colon - 1,
    };
    dst->value = strip(v);

    return 0;
}


struct http_header_field* get_header(struct http_request* req, char* name) {
    size_t len = strlen(name);

    for (int i = 0; i < req->header_count; i++) {
        if (req->headers[i].name.len != len) continue;

        // tolowerしてから比較する
        for (int j = 0; j < req->headers[i].name.len; j++) {
            char h = (char)tolower((unsigned char)req->headers[i].name.ptr[j]);
            char v = (char)tolower((unsigned char)name[j]);
            if (h != v) break;

            if (j + 1 == req->headers[i].name.len) {
                return &req->headers[i];
            }
        }
    }
    return NULL;
}

int slice_to_size_t(struct str_slice s, size_t* out) {
    if (s.len == 0) return -1;
    size_t v = 0;
    for (size_t i = 0; i < s.len; i++) {
        if (!isdigit((unsigned char)s.ptr[i])) return -1;
        size_t d = s.ptr[i] - '0';
        if (v > (SIZE_MAX - d) / 10) return -1; // overflow
        v = v * 10 + d;
    }
    *out = v;
    return 0;
}

int slice_eq(struct str_slice s, const char* str) {
    size_t n = strlen(str);
    return s.len == n && memcmp(s.ptr, str, n) == 0;
}

int parse_query_param(char* buf, size_t len, struct http_query_param* dst) {
    char* c_eq = findchr(buf, len, '=');
    if (c_eq == NULL) return -1;

    struct str_slice name = {
        .ptr = buf,
        .len = c_eq - buf,
    };
    dst->name = name;

    struct str_slice v = {
        .ptr = c_eq + 1,
        .len = buf+len - c_eq - 1,
    };
    dst->value = v;

    return 0;
}

int parse_query(struct http_request* req) {
    char* next_param = req->query.ptr;
    const char* pos_end = req->query.ptr + req->query.len;
    int i = 0;

    while (next_param < pos_end && i < 16) {
        char* pos_amp = findchr(next_param, pos_end - next_param, '&');
        if (pos_amp == NULL) pos_amp = pos_end;
        if (pos_amp == next_param) { // 空要素は読み飛ばす. 先頭に`&`あるいは`&&`などのとき
            next_param = pos_amp + 1;
            continue;
        }
        if (parse_query_param(next_param, pos_amp - next_param, &req->params[i]) == -1) {
            return -1;
        }

        next_param = pos_amp + 1;
        i++;
    }
    req->param_count = i;
    return 0;
}

int parse_request_target(struct http_request* req) {
    req->path = req->target;
    if (req->target.len <= 1) {
        return 0;
    }

    char* q = findchr(req->target.ptr, req->target.len, '?');
    if (q == NULL) return 0;

    struct str_slice path = {
        .ptr = req->target.ptr,
        .len = q - req->target.ptr,
    };
    req->path = path;

    struct str_slice query = {
        .ptr = q + 1,
        .len = req->target.len - (q + 1 - req->target.ptr),
    };
    req->query = query;

    // query to params
    return parse_query(req);
}

int parse_http_request_head(char* buf, struct http_request* req) {
    char* end_of_header = strstr(buf, "\r\n\r\n");
    if (end_of_header == NULL) return -1;

    int i = 0;
    char* next_line = buf; // requestの先頭から
    while (i < 32 && next_line < end_of_header) {
        char* line_tail = strstr(next_line, "\r\n");
        if (line_tail == NULL) break;

        struct str_slice line;
        line.ptr = next_line;
        line.len = line_tail - next_line;

        req->header_lines[i] = line;
        next_line = line_tail + 2;
        i++;
    }
    req->header_line_count = i;

    if (parse_request_line(req) == -1) {
        return -1;
    }

    for (size_t i = 1; i < req->header_line_count; i++) {
        if (parse_request_header_field(
                    req->header_lines[i].ptr,
                    req->header_lines[i].len,
                    &req->headers[i-1] // headers側は0始まりにする
                    ) == -1) {
            return -1;
        }
    }
    req->header_count = req->header_line_count - 1;

    if (parse_request_target(req) == -1) {
        return -1;
    }

    return end_of_header - buf + 4; // +4 is "\r\n\r\n" length
}
