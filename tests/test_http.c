// http.c の純粋なパース関数に対する単体テスト
//
// ビルド・実行: make unit-test
// 外部ライブラリなし．assert ベース．失敗すると abort する．

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/http.h"

// http.h に公開されていない内部ヘルパーも直接テストする
struct str_slice strip(struct str_slice slice);
char* findchr(char* buf, size_t len, char c);
char* findstr(char* buf, size_t len, char* str);

// ---- helpers --------------------------------------------------------------
// slice_eq は http.h の公開関数を使う

// リテラルを書き込み可能なバッファにコピーして parse する
static int parse(const char* raw, char* buf, size_t buf_size, struct http_request* req) {
    memset(buf, 0, buf_size);
    strncpy(buf, raw, buf_size - 1);
    memset(req, 0, sizeof *req);
    return parse_http_request_head(buf, req);
}

#define RUN(fn) do { printf("  %-48s", #fn); fn(); printf("ok\n"); } while (0)

// ---- request line ---------------------------------------------------------

static void test_request_line_get(void) {
    char buf[256]; struct http_request req;
    int rc = parse("GET /calc HTTP/1.1\r\n\r\n", buf, sizeof buf, &req);

    assert(rc == 22);  // "\r\n\r\n" を含む消費バイト数
    assert(req.method == HTTP_METHOD_GET);
    assert(slice_eq(req.target, "/calc"));
    assert(req.version == HTTP_VERSION_1_1);
}

static void test_request_line_post_http10(void) {
    char buf[256]; struct http_request req;
    int rc = parse("POST / HTTP/1.0\r\n\r\n", buf, sizeof buf, &req);

    assert(rc > 0);
    assert(req.method == HTTP_METHOD_POST);
    assert(slice_eq(req.target, "/"));
    assert(req.version == HTTP_VERSION_1_0);
}

static void test_request_line_http09(void) {
    char buf[256]; struct http_request req;
    parse("GET / HTTP/0.9\r\n\r\n", buf, sizeof buf, &req);

    assert(req.version == HTTP_VERSION_0_9);
}

static void test_request_line_unknown_method_and_version(void) {
    char buf[256]; struct http_request req;
    int rc = parse("DELETE / HTTP/2.0\r\n\r\n", buf, sizeof buf, &req);

    assert(rc > 0);  // 構文としては正しいので成功
    assert(req.method == HTTP_METHOD_UNKNOWN);
    assert(req.version == HTTP_VERSION_UNKNOWN);
}

static void test_request_line_missing_version_fails(void) {
    char buf[256]; struct http_request req;
    int rc = parse("GET /calc\r\n\r\n", buf, sizeof buf, &req);

    assert(rc == -1);
    assert(req.method == HTTP_METHOD_UNKNOWN);  // ゼロ初期化のまま = UNKNOWN
}

static void test_request_line_method_only_fails(void) {
    char buf[256]; struct http_request req;
    assert(parse("GET\r\n\r\n", buf, sizeof buf, &req) == -1);
}

static void test_request_line_does_not_read_next_line(void) {
    // version が無い行で，次の行の空白を拾ってはいけない
    char buf[256]; struct http_request req;
    int rc = parse("GET /ca\r\nA b\r\n\r\n", buf, sizeof buf, &req);

    assert(rc == -1);
    assert(req.target.len == 0);
}

static void test_no_header_end_fails(void) {
    char buf[256]; struct http_request req;
    assert(parse("GET / HTTP/1.1\r\nHost: a\r\n", buf, sizeof buf, &req) == -1);
}

static void test_empty_input_fails(void) {
    char buf[256]; struct http_request req;
    assert(parse("", buf, sizeof buf, &req) == -1);
}

// ---- path / query ---------------------------------------------------------

static void test_target_with_query(void) {
    char buf[256]; struct http_request req;
    parse("GET /calc?a=1 HTTP/1.1\r\n\r\n", buf, sizeof buf, &req);

    assert(slice_eq(req.target, "/calc?a=1"));  // target は生のまま
    assert(slice_eq(req.path, "/calc"));
    assert(slice_eq(req.query, "a=1"));
}

static void test_target_without_query(void) {
    char buf[256]; struct http_request req;
    parse("GET /calc HTTP/1.1\r\n\r\n", buf, sizeof buf, &req);

    assert(slice_eq(req.path, "/calc"));
    assert(req.query.len == 0);
}

static void test_target_empty_query(void) {
    char buf[256]; struct http_request req;
    parse("GET /calc? HTTP/1.1\r\n\r\n", buf, sizeof buf, &req);

    assert(slice_eq(req.path, "/calc"));
    assert(req.query.len == 0);
}

static void test_target_splits_at_first_question_mark(void) {
    char buf[256]; struct http_request req;
    parse("GET /a?b?c HTTP/1.1\r\n\r\n", buf, sizeof buf, &req);

    assert(slice_eq(req.path, "/a"));
    assert(slice_eq(req.query, "b?c"));
}

static void test_target_root(void) {
    char buf[256]; struct http_request req;
    parse("GET / HTTP/1.1\r\n\r\n", buf, sizeof buf, &req);

    assert(slice_eq(req.path, "/"));
    assert(req.query.len == 0);
}

static void test_target_leading_question_mark(void) {
    // '?' が先頭でも path.len が underflow してはいけない
    char buf[256]; struct http_request req;
    parse("GET ?a=1 HTTP/1.1\r\n\r\n", buf, sizeof buf, &req);

    assert(req.path.len == 0);
    assert(slice_eq(req.query, "a=1"));
}

// ---- query params ---------------------------------------------------------

static void test_query_two_params(void) {
    char buf[256]; struct http_request req;
    int rc = parse("GET /calc?a=1&b=2 HTTP/1.1\r\n\r\n", buf, sizeof buf, &req);

    assert(rc > 0);
    assert(req.param_count == 2);
    assert(slice_eq(req.params[0].name, "a"));
    assert(slice_eq(req.params[0].value, "1"));
    assert(slice_eq(req.params[1].name, "b"));
    assert(slice_eq(req.params[1].value, "2"));
}

static void test_query_single_param(void) {
    char buf[256]; struct http_request req;
    parse("GET /calc?a=1 HTTP/1.1\r\n\r\n", buf, sizeof buf, &req);

    assert(req.param_count == 1);
    assert(slice_eq(req.params[0].name, "a"));
    assert(slice_eq(req.params[0].value, "1"));
}

static void test_query_none(void) {
    char buf[256]; struct http_request req;
    parse("GET /calc HTTP/1.1\r\n\r\n", buf, sizeof buf, &req);

    assert(req.param_count == 0);
}

static void test_query_empty(void) {
    char buf[256]; struct http_request req;
    parse("GET /calc? HTTP/1.1\r\n\r\n", buf, sizeof buf, &req);

    assert(req.param_count == 0);
}

static void test_query_skips_empty_segment(void) {
    char buf[256]; struct http_request req;
    int rc = parse("GET /calc?a=1&&b=2 HTTP/1.1\r\n\r\n", buf, sizeof buf, &req);

    assert(rc > 0);
    assert(req.param_count == 2);
    assert(slice_eq(req.params[0].name, "a"));
    assert(slice_eq(req.params[1].name, "b"));
}

static void test_query_skips_leading_and_trailing_amp(void) {
    char buf[256]; struct http_request req;
    int rc = parse("GET /calc?&a=1& HTTP/1.1\r\n\r\n", buf, sizeof buf, &req);

    assert(rc > 0);
    assert(req.param_count == 1);
    assert(slice_eq(req.params[0].name, "a"));
    assert(slice_eq(req.params[0].value, "1"));
}

static void test_query_empty_value(void) {
    char buf[256]; struct http_request req;
    parse("GET /calc?a= HTTP/1.1\r\n\r\n", buf, sizeof buf, &req);

    assert(req.param_count == 1);
    assert(slice_eq(req.params[0].name, "a"));
    assert(req.params[0].value.len == 0);
}

static void test_query_name_only(void) {
    // "?flag" の形: '=' が無ければ name のみで value は空
    char buf[256]; struct http_request req;
    int rc = parse("GET /calc?flag HTTP/1.1\r\n\r\n", buf, sizeof buf, &req);

    assert(rc > 0);
    assert(req.param_count == 1);
    assert(slice_eq(req.params[0].name, "flag"));
    assert(req.params[0].value.len == 0);
}

static void test_query_name_only_mixed(void) {
    char buf[256]; struct http_request req;
    int rc = parse("GET /calc?a&b=2 HTTP/1.1\r\n\r\n", buf, sizeof buf, &req);

    assert(rc > 0);
    assert(req.param_count == 2);
    assert(slice_eq(req.params[0].name, "a"));
    assert(req.params[0].value.len == 0);
    assert(slice_eq(req.params[1].name, "b"));
    assert(slice_eq(req.params[1].value, "2"));
}

static void test_query_splits_at_first_equals(void) {
    char buf[256]; struct http_request req;
    parse("GET /calc?a=b=c HTTP/1.1\r\n\r\n", buf, sizeof buf, &req);

    assert(req.param_count == 1);
    assert(slice_eq(req.params[0].name, "a"));
    assert(slice_eq(req.params[0].value, "b=c"));
}

static void test_query_caps_at_16_params(void) {
    // 17 個渡しても params[16] に書き込まず 16 個で止まる
    char buf[512]; struct http_request req;
    char raw[512] = "GET /calc?";
    for (int k = 0; k < 17; k++) {
        char pair[16];
        snprintf(pair, sizeof pair, "%sk%d=v", k ? "&" : "", k);
        strcat(raw, pair);
    }
    strcat(raw, " HTTP/1.1\r\n\r\n");

    int rc = parse(raw, buf, sizeof buf, &req);
    assert(rc > 0);
    assert(req.param_count == 16);
    assert(slice_eq(req.params[15].name, "k15"));
}

// ---- header fields --------------------------------------------------------

static void test_headers_basic(void) {
    char buf[256]; struct http_request req;
    int rc = parse("GET / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 42\r\n\r\n",
                   buf, sizeof buf, &req);

    assert(rc > 0);
    assert(req.header_line_count == 3);
    assert(req.header_count == 2);
    assert(slice_eq(req.headers[0].name, "Host"));
    assert(slice_eq(req.headers[0].value, "localhost"));
    assert(slice_eq(req.headers[1].name, "Content-Length"));
    assert(slice_eq(req.headers[1].value, "42"));
}

static void test_headers_none(void) {
    char buf[256]; struct http_request req;
    int rc = parse("GET / HTTP/1.1\r\n\r\n", buf, sizeof buf, &req);

    assert(rc > 0);
    assert(req.header_count == 0);
}

static void test_headers_ows_stripped(void) {
    char buf[256]; struct http_request req;
    parse("GET / HTTP/1.1\r\nContent-Length:   42   \r\n\r\n", buf, sizeof buf, &req);

    assert(slice_eq(req.headers[0].value, "42"));
}

static void test_headers_no_ows(void) {
    char buf[256]; struct http_request req;
    parse("GET / HTTP/1.1\r\nY:a\r\n\r\n", buf, sizeof buf, &req);

    assert(slice_eq(req.headers[0].name, "Y"));
    assert(slice_eq(req.headers[0].value, "a"));
}

static void test_headers_empty_value(void) {
    char buf[256]; struct http_request req;
    parse("GET / HTTP/1.1\r\nX:\r\n\r\n", buf, sizeof buf, &req);

    assert(slice_eq(req.headers[0].name, "X"));
    assert(req.headers[0].value.len == 0);
}

static void test_headers_whitespace_only_value(void) {
    char buf[256]; struct http_request req;
    parse("GET / HTTP/1.1\r\nX:   \r\n\r\n", buf, sizeof buf, &req);

    assert(req.headers[0].value.len == 0);
}

static void test_headers_value_with_colon(void) {
    // 最初の ':' で分割し，value 側の ':' は残す
    char buf[256]; struct http_request req;
    parse("GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n", buf, sizeof buf, &req);

    assert(slice_eq(req.headers[0].name, "Host"));
    assert(slice_eq(req.headers[0].value, "localhost:8080"));
}

static void test_headers_missing_colon_fails(void) {
    char buf[256]; struct http_request req;
    int rc = parse("GET / HTTP/1.1\r\nNoColon\r\nHost: a\r\n\r\n", buf, sizeof buf, &req);

    assert(rc == -1);
}

static void test_headers_does_not_read_next_line(void) {
    // ':' の無い行で，次の行の ':' を拾って name が2行にまたがってはいけない
    char buf[256]; struct http_request req;
    int rc = parse("GET / HTTP/1.1\r\nBroken\r\nHost: a\r\n\r\n", buf, sizeof buf, &req);

    assert(rc == -1);
    assert(req.headers[0].name.len == 0);
}

// ---- get_header -----------------------------------------------------------

static void test_get_header_exact(void) {
    char buf[256]; struct http_request req;
    parse("GET / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 42\r\n\r\n",
          buf, sizeof buf, &req);

    struct http_header_field* f = get_header(&req, "Content-Length");
    assert(f != NULL);
    assert(slice_eq(f->value, "42"));
}

static void test_get_header_case_insensitive(void) {
    char buf[256]; struct http_request req;
    parse("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n", buf, sizeof buf, &req);

    assert(get_header(&req, "host") != NULL);
    assert(get_header(&req, "HOST") != NULL);
    assert(get_header(&req, "hOsT") != NULL);
}

static void test_get_header_not_found(void) {
    char buf[256]; struct http_request req;
    parse("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n", buf, sizeof buf, &req);

    assert(get_header(&req, "X-None") == NULL);
}

static void test_get_header_prefix_does_not_match(void) {
    char buf[256]; struct http_request req;
    parse("GET / HTTP/1.1\r\nContent-Length: 42\r\n\r\n", buf, sizeof buf, &req);

    assert(get_header(&req, "Content-Lengt") == NULL);   // 1文字短い
    assert(get_header(&req, "Content-Length2") == NULL); // 1文字長い
}

static void test_get_header_returns_first_match(void) {
    char buf[256]; struct http_request req;
    parse("GET / HTTP/1.1\r\nX: 1\r\nX: 2\r\n\r\n", buf, sizeof buf, &req);

    struct http_header_field* f = get_header(&req, "X");
    assert(f != NULL);
    assert(slice_eq(f->value, "1"));
}

static void test_get_header_on_empty_request(void) {
    struct http_request req = {0};
    assert(get_header(&req, "Host") == NULL);
}

// ---- slice_to_size_t ------------------------------------------------------

static struct str_slice S(const char* s) {
    struct str_slice sl = { .ptr = s, .len = strlen(s) };
    return sl;
}

static void test_slice_to_size_t_basic(void) {
    size_t v = 999;
    assert(slice_to_size_t(S("42"), &v) == 0);
    assert(v == 42);
}

static void test_slice_to_size_t_zero(void) {
    size_t v = 999;
    assert(slice_to_size_t(S("0"), &v) == 0);
    assert(v == 0);
}

static void test_slice_to_size_t_respects_len(void) {
    // slice の範囲外にある文字は読まない
    size_t v = 999;
    struct str_slice s = { .ptr = "42xyz", .len = 2 };
    assert(slice_to_size_t(s, &v) == 0);
    assert(v == 42);
}

static void test_slice_to_size_t_non_digit_fails(void) {
    size_t v = 999;
    assert(slice_to_size_t(S("4a"), &v) == -1);
    assert(v == 999);  // 失敗時は out を触らない
}

static void test_slice_to_size_t_negative_fails(void) {
    size_t v = 999;
    assert(slice_to_size_t(S("-1"), &v) == -1);
}

static void test_slice_to_size_t_empty_fails(void) {
    size_t v = 999;
    assert(slice_to_size_t(S(""), &v) == -1);
}

static void test_slice_to_size_t_max(void) {
    size_t v = 0;
    assert(slice_to_size_t(S("18446744073709551615"), &v) == 0);  // SIZE_MAX
    assert(v == SIZE_MAX);
}

static void test_slice_to_size_t_overflow_fails(void) {
    size_t v = 999;
    assert(slice_to_size_t(S("18446744073709551616"), &v) == -1);  // SIZE_MAX + 1
    assert(slice_to_size_t(S("99999999999999999999999"), &v) == -1);
}

// ---- slice_eq -------------------------------------------------------------

static void test_slice_eq_match(void) {
    assert(slice_eq(S("/calc"), "/calc"));
}

static void test_slice_eq_prefix_does_not_match(void) {
    assert(!slice_eq(S("/calculator"), "/calc"));  // 長い方
    assert(!slice_eq(S("/cal"), "/calc"));         // 短い方
}

static void test_slice_eq_respects_len(void) {
    // slice の範囲外の文字は比較に含めない
    struct str_slice s = { .ptr = "/calc?a=1", .len = 5 };
    assert(slice_eq(s, "/calc"));
}

static void test_slice_eq_empty(void) {
    assert(slice_eq(S(""), ""));
    assert(!slice_eq(S(""), "/"));
    assert(!slice_eq(S("/"), ""));
}

static void test_slice_eq_case_sensitive(void) {
    assert(!slice_eq(S("/Calc"), "/calc"));
}

// ---- strip ----------------------------------------------------------------

static void test_strip_both_sides(void) {
    struct str_slice s = { .ptr = "  abc  ", .len = 7 };
    assert(slice_eq(strip(s), "abc"));
}

static void test_strip_nothing_to_strip(void) {
    struct str_slice s = { .ptr = "abc", .len = 3 };
    assert(slice_eq(strip(s), "abc"));
}

static void test_strip_all_spaces(void) {
    struct str_slice s = { .ptr = "   ", .len = 3 };
    assert(strip(s).len == 0);
}

static void test_strip_single_space(void) {
    struct str_slice s = { .ptr = " ", .len = 1 };
    assert(strip(s).len == 0);
}

static void test_strip_empty(void) {
    struct str_slice s = { .ptr = "", .len = 0 };
    assert(strip(s).len == 0);
}

// ---- findchr / findstr ----------------------------------------------------

static void test_findchr_found(void) {
    char buf[] = "Host: a";
    assert(findchr(buf, sizeof buf - 1, ':') == buf + 4);
}

static void test_findchr_bounded_by_len(void) {
    // len の範囲外にある ':' は見つけてはいけない
    char buf[] = "Host: a";
    assert(findchr(buf, 4, ':') == NULL);
}

static void test_findstr_found(void) {
    char buf[] = "ab\r\ncd";
    assert(findstr(buf, sizeof buf - 1, "\r\n") == buf + 2);
}

static void test_findstr_at_end(void) {
    // パターンがちょうど末尾にあるケース
    char buf[] = "ab\r\n";
    assert(findstr(buf, sizeof buf - 1, "\r\n") == buf + 2);
}

static void test_findstr_bounded_by_len(void) {
    char buf[] = "ab\r\ncd";
    assert(findstr(buf, 3, "\r\n") == NULL);  // "\r" までしか範囲に入っていない
}

static void test_findstr_not_found(void) {
    char buf[] = "abcd";
    assert(findstr(buf, sizeof buf - 1, "\r\n") == NULL);
}

static void test_findstr_empty_pattern(void) {
    char buf[] = "abcd";
    assert(findstr(buf, sizeof buf - 1, "") == NULL);
}

// ---- main -----------------------------------------------------------------

int main(void) {
    printf("request line\n");
    RUN(test_request_line_get);
    RUN(test_request_line_post_http10);
    RUN(test_request_line_http09);
    RUN(test_request_line_unknown_method_and_version);
    RUN(test_request_line_missing_version_fails);
    RUN(test_request_line_method_only_fails);
    RUN(test_request_line_does_not_read_next_line);
    RUN(test_no_header_end_fails);
    RUN(test_empty_input_fails);

    printf("path / query\n");
    RUN(test_target_with_query);
    RUN(test_target_without_query);
    RUN(test_target_empty_query);
    RUN(test_target_splits_at_first_question_mark);
    RUN(test_target_root);
    RUN(test_target_leading_question_mark);

    printf("query params\n");
    RUN(test_query_two_params);
    RUN(test_query_single_param);
    RUN(test_query_none);
    RUN(test_query_empty);
    RUN(test_query_skips_empty_segment);
    RUN(test_query_skips_leading_and_trailing_amp);
    RUN(test_query_empty_value);
    RUN(test_query_name_only);
    RUN(test_query_name_only_mixed);
    RUN(test_query_splits_at_first_equals);
    RUN(test_query_caps_at_16_params);

    printf("header fields\n");
    RUN(test_headers_basic);
    RUN(test_headers_none);
    RUN(test_headers_ows_stripped);
    RUN(test_headers_no_ows);
    RUN(test_headers_empty_value);
    RUN(test_headers_whitespace_only_value);
    RUN(test_headers_value_with_colon);
    RUN(test_headers_missing_colon_fails);
    RUN(test_headers_does_not_read_next_line);

    printf("get_header\n");
    RUN(test_get_header_exact);
    RUN(test_get_header_case_insensitive);
    RUN(test_get_header_not_found);
    RUN(test_get_header_prefix_does_not_match);
    RUN(test_get_header_returns_first_match);
    RUN(test_get_header_on_empty_request);

    printf("slice_to_size_t\n");
    RUN(test_slice_to_size_t_basic);
    RUN(test_slice_to_size_t_zero);
    RUN(test_slice_to_size_t_respects_len);
    RUN(test_slice_to_size_t_non_digit_fails);
    RUN(test_slice_to_size_t_negative_fails);
    RUN(test_slice_to_size_t_empty_fails);
    RUN(test_slice_to_size_t_max);
    RUN(test_slice_to_size_t_overflow_fails);

    printf("slice_eq\n");
    RUN(test_slice_eq_match);
    RUN(test_slice_eq_prefix_does_not_match);
    RUN(test_slice_eq_respects_len);
    RUN(test_slice_eq_empty);
    RUN(test_slice_eq_case_sensitive);

    printf("strip\n");
    RUN(test_strip_both_sides);
    RUN(test_strip_nothing_to_strip);
    RUN(test_strip_all_spaces);
    RUN(test_strip_single_space);
    RUN(test_strip_empty);

    printf("findchr / findstr\n");
    RUN(test_findchr_found);
    RUN(test_findchr_bounded_by_len);
    RUN(test_findstr_found);
    RUN(test_findstr_at_end);
    RUN(test_findstr_bounded_by_len);
    RUN(test_findstr_not_found);
    RUN(test_findstr_empty_pattern);

    printf("all tests passed\n");
    return 0;
}
