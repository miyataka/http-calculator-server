# ここは学習メモとか単なる日報とかを書いていくところ．

記載順は日付の降順

# 20260923

大きなTODOs
- [ ] query paramsに対応する
- [ ] response応答を関数化
- [ ] POSTに対応する
    - request bodyを読めるようにする
- [ ] connection: closeを固定でいれる
- [ ] keep-alive対応をいれる


`http headerをparseする`をやるための小さなTODOs
- [ ] `recv_http_body(fd, buf, size, already_have, content_length)` を実装する
    - head と一緒に届いた body の余りを差し引き、不足分だけ recv
- [ ] `receive_http_request(fd, buf, size, req)` を実装する
    - recv_http_head → parse_http_request_head → (Content-Length があれば) recv_http_body
    - `req->body` を埋める
- [ ] main.c の recv / parse 呼び出しを `receive_http_request` 1回に置き換える
- [ ] POST /calc で body を読んで応答する

`query string を parse する`の小さなTODOs
- [x] `struct query_param { struct str_slice name; struct str_slice value; }` と
      `params[16]` / `param_count` を http_request に追加する
- [ ] `parse_query_param(str_slice pair, struct query_param* out)` を実装する
    - `a=1` を最初の `=` で name / value に分ける
    - `=` が無ければ name のみで value は空 slice（`?flag` のような形）
    - テスト: `a=1`，`a=`，`a`，`a=b=c` → value `b=c`
- [ ] `parse_query(req)` を実装し，`query` を `&` で区切って `parse_query_param` に通す
    - 空要素（`a=1&&b=2`）は読み飛ばす
    - `params[16]` を越えたら -1
    - テスト: `a=1&b=2`，空 query → count 0，`a=1&&b=2`，先頭・末尾の `&`
- [ ] `get_query_param(req, name)` を実装する
    - `get_header` と同じ形．ただしクエリ名は大文字小字を区別する（`slice_eq` でよい）
    - 見つからなければ NULL
- [ ] `slice_to_long(str_slice, long* out)` を実装する
    - `slice_to_size_t` の符号付き版．先頭の `-` を許す
    - テスト: `42`，`-42`，`-`，`4a`，空，overflow
- [ ] `calc_handler` で `a` / `b` / `op` を取り出して計算し，結果を body に入れて返す
    - op は `add` / `sub` / `mul` / `div` の4つから
    - param 不足・数値化失敗・未知の op・ゼロ除算は 400
    - body は `snprintf` で組み立て，Content-Length も実長から計算する
    - test.sh に `curl 'localhost:8080/calc?a=1&b=2&op=add'` → `3` のケースを足す
- [ ] （後回し）`%20` などの percent-decoding
    - 今は数値と英字しか使わないので，POST の form 対応と一緒にやる

# 20260922

とにかくやることがたくさんある．勉強のためにあくまでも手で書くのでとても時間がかかる．
バグがないかAIにreviewしてもらい，めちゃくちゃ指摘をもらって片っ端から直していく．
今日初めてatoiを使うときに，C言語に`<stdlib.h>`というヘッダがあることを知った．
全然標準ライブラリのスコープとあってなくて命名にワロタ．歴史的経緯なんだろうな．
str_sliceというstructを作り，やっと文字列処理がまとも（定型的）になってきた．
bufferを読み取り専用として共有する形で扱うことに慣れていない．まぁたまにGo風味も感じたので，
GoのコアチームはCのライブラリとのインターフェースの一致をある程度気にしているのだろう．

大きなTODOs
- [x] http headerをparseする．
    - とりあえずhostとcontent-lengthのみ
    - その他
- [ ] query paramsに対応する
- [ ] response応答を関数化
- [ ] POSTに対応する
    - request bodyを読めるようにする
- [ ] connection: closeを固定でいれる
- [ ] keep-alive対応をいれる

`http headerをparseする`をやるための小さなTODOs
- [x] `struct str_slice { const char* ptr; size_t len; }` を http.h に追加する
- [x] `struct http_header` を `struct http_request` にリネームし、target を str_slice
にする
    - `target` / `target_len` → `struct str_slice target`
    - main.c の参照箇所も追従
- [x] `parse_http_header` の行分割バグを直す
    - `char** header_lines[32]` → 行ごとの str_slice 配列に
    - 行長は `end_of_line - next_line` で持つ
- [x] `parse_request_line(const char* line, size_t len, struct http_request* req)`
を切り出す
    - 空白で3分割して method / target / version を埋める
    - 失敗時は -1 を返す
- [x] `parse_http_request_head(buf, len, req)` に改名し、消費バイト数（`\r\n\r\n`
含む）を返す
- [x] `struct http_header { str_slice name; str_slice value; }` と `headers[32]` /
`header_count` を http_request に追加
- [x] `parse_request_header_field(line, len, struct http_header* out)` を実装する
    - `:` で分割、前後の空白（OWS）を落とす
- [x] `parse_http_request_head` の中で2行目以降を `parse_request_header_field` に通して headers
に詰める
- [x] `get_header(const struct http_request* req, const char* name)` を実装する
    - 大文字小文字を無視して比較（`slice_eq_ci`）
    - 見つからなければ NULL
- [x] `slice_to_size_t(str_slice, size_t* out)` を実装し、Content-Length を数値化する
- [x] パース失敗時に 400 Bad Request を返す `bad_request(fd)` を http_handler に追加し
main.c で使う
- [ ] `recv_http_body(fd, buf, size, already_have, content_length)` を実装する
    - head と一緒に届いた body の余りを差し引き、不足分だけ recv
- [ ] `receive_http_request(fd, buf, size, req)` を実装する
    - recv_http_head → parse_http_request_head → (Content-Length があれば) recv_http_body
    - `req->body` を埋める
- [ ] main.c の recv / parse 呼び出しを `receive_http_request` 1回に置き換える
- [ ] POST /calc で body を読んで応答する

`query string を parse する`の小さなTODOs
- [x] `struct http_request` に `struct str_slice path;` と `struct str_slice query;` を追加する
    - `target` は生のまま残す（`/calc?a=1` 全体）
- [x] `parse_request_target(req)` を実装し，`target` を最初の `?` で `path` と `query` に分ける
    - `?` が無ければ `path = target`, `query` は空 slice
    - `parse_request_line` の末尾から呼ぶ
    - テスト: `/calc?a=1` → path `/calc` / query `a=1`，`/calc` → query 空，`/calc?` → query空，`/a?b?c` → query `b?c`
- [x] main.c の routing を `target` ではなく `path` で判定するように変える
    - 今の `memcmp(target, "/calc", 5)` は `/calculator` にも一致するので `path.len == 5` の完全一致にする
    - `char target[]` へのコピーをやめて slice のまま比較する
- [ ] `struct query_param { struct str_slice name; struct str_slice value; }` と
      `params[16]` / `param_count` を http_request に追加する
- [ ] `parse_query_param(str_slice pair, struct query_param* out)` を実装する
    - `a=1` を最初の `=` で name / value に分ける
    - `=` が無ければ name のみで value は空 slice（`?flag` のような形）
    - テスト: `a=1`，`a=`，`a`，`a=b=c` → value `b=c`
- [ ] `parse_query(req)` を実装し，`query` を `&` で区切って `parse_query_param` に通す
    - 空要素（`a=1&&b=2`）は読み飛ばす
    - `params[16]` を越えたら -1
    - テスト: `a=1&b=2`，空 query → count 0，`a=1&&b=2`，先頭・末尾の `&`
- [ ] `get_query_param(req, name)` を実装する
    - `get_header` と同じ形．ただしクエリ名は大文字小字を区別する（`slice_eq` でよい）
    - 見つからなければ NULL
- [ ] `slice_to_long(str_slice, long* out)` を実装する
    - `slice_to_size_t` の符号付き版．先頭の `-` を許す
    - テスト: `42`，`-42`，`-`，`4a`，空，overflow
- [ ] `calc_handler` で `a` / `b` / `op` を取り出して計算し，結果を body に入れて返す
    - op は `add` / `sub` / `mul` / `div` の4つから
    - param 不足・数値化失敗・未知の op・ゼロ除算は 400
    - body は `snprintf` で組み立て，Content-Length も実長から計算する
    - test.sh に `curl 'localhost:8080/calc?a=1&b=2&op=add'` → `3` のケースを足す
- [ ] （後回し）`%20` などの percent-decoding
    - 今は数値と英字しか使わないので，POST の form 対応と一緒にやる

# 20260921
今日はここまでで作ったtcp-echo-serverをhttp-serverにしていく作業を行う
LLMと相談して以下のステップを刻むことにする
- [x] 固定レスポンスを返す
- [x] http request header終端を認識する
    - 文字列の初期化漏れでバグらせた．
- [x] request lineだけ（requestの先頭一行）だけparseする
- [x] routingに対応する
- [x] error応答を入れる
    - not found
- [ ] http headerをparseする．
    - とりあえずhostとcontent-lengthのみ
    - その他
- [ ] response応答を関数化
- [ ] POSTに対応する
    - request bodyを読めるようにする
- [ ] connection: closeを固定でいれる
- [ ] keep-alive対応をいれる

# 20260920

LLMに聞いたところ，以下のsystem callを使うとTCPサーバーを作れるとのこと．
これらをmanで読んでいく

TCPサーバー
- socket()
- setsockopt(SO_REUSEADDR)
- bind()
- listen()
- accept()
- recv()/read()
- send()/write()
- close()

manコマンドでシステムコールを調べるときは，2を指定する． `man 2 socket`

まずはTCP echoサーバー作れとのことだったので従う

さっきのシステムコールを記載の順序で使うらしい
1. socket()でsocket（に対応したfd）を取得
1. setsockoptでsocketにオプションを指定する. ip/tcp/udpなどいろいろ指定できる
    - ipのオプション
    - tcpのオプション
    - udpのオプション
1. bindでsocketにip-addressとportの組みを紐づける（あるいはunix-socketのfilepath）
1. listenで接続の受付を開始する．キューの長さを指定できる．
1. acceptで接続を別のsocket（に対応したfd）として受け取る
1. recv/readで接続から読み出す．TCPの場合はbyte-streamとして読み出せる
1. send/writeで接続へ書き込む．
1. closeでsocket（と紐づくfd）を解放する

## socket
とりあえず以下はプログラムを書くときに使うと思うのでコピペしておく
 ```
 #include <sys/socket.h>

 int
 socket(int domain, int type, int protocol);
```

引数はdomain, type, protocol, いずれもint

domainは，PF_LOCAL, PF_INET, PF_INET6 をとりいそぎ覚えてればよさそうだ

typeは，SOCK_STREAM, SOCK_DGRAM, SOCK_RAWの3つがある
SOCK_STREAMは，TCPに対応していそう
SOCK_DGRAMは，UDPに対応していそう
SOCK_RAWは，internal network protocolsとからしい. unix socketのときに使う感じかな．


protocolsは，プロトコル番号というのを渡すらしい. /etc/protocolsというファイルを初めてみた
これは，定数があるのか，直接数字を指定するかは気になる．が後回しにする


## setsockopt(SO_REUSEADDR)

とりあえず，socketのオプション指定なのだろう
```
SYNOPSIS
     #include <sys/socket.h>

     int
     getsockopt(int socket, int level, int option_name, void *restrict option_value, socklen_t *restrict option_len);

     int
     setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len);
```


## bind

bindはsocketにunix socketとなるfilepathか，ip-address:port，のどちらかを紐づける

```
SYNOPSIS
     #include <sys/socket.h>

     int
     bind(int socket, const struct sockaddr *address, socklen_t address_len);
```

## listen

listenは接続の受付を開始する．第二引数はキューのサイズ．

```
SYNOPSIS
     #include <sys/socket.h>

     int
     listen(int socket, int backlog);
```

## accept

acceptはlistenしたsocketから，接続確立したsocket(connection)を別socket(fd)として受け取る．
第二第三引数は，接続先のaddress, portなどを受け取りたいときに使う
```
SYNOPSIS
     #include <sys/socket.h>

     int
     accept(int socket, struct sockaddr *restrict address, socklen_t *restrict address_len);
```

## recv/read
recv/readはacceptで取得したsocketから，byte streamを読み出す．読み出すbyte数を指定できるが，指定した数が読み込めないときもある．
返り値が`0`になるまで繰り返し読むことでEOFとなったことを検知できる．

```
SYNOPSIS
     #include <sys/socket.h>

     ssize_t
     recv(int socket, void *buffer, size_t length, int flags);

     ssize_t
     recvfrom(int socket, void *restrict buffer, size_t length, int flags, struct sockaddr *restrict address,
         socklen_t *restrict address_len);

     ssize_t
     recvmsg(int socket, struct msghdr *message, int flags);
```

## send/write
socket/connectionに書き込む．書き込むbyte数を指定できるが，一度に書き込めないときもあるので，全部書き込むためにはループする必要がある．
```
SYNOPSIS
     #include <sys/socket.h>

     ssize_t
     send(int socket, const void *buffer, size_t length, int flags);

     ssize_t
     sendmsg(int socket, const struct msghdr *message, int flags);

     ssize_t
     sendto(int socket, const void *buffer, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
```

## close
socketを閉じる
