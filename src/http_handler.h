#include <sys/types.h>

struct calc_expr {
    long lhs;
    char op;
    long rhs;
};

ssize_t calc_handler(int socket, struct http_request req);
ssize_t not_found(int socket);
ssize_t bad_request(int socket);

int eval_calc_expr(const struct calc_expr* e, long* out);
