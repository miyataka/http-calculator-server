# ここは学習メモとか単なる日報とかを書いていくところ．

記載順は日付の降順

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
