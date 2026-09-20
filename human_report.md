# ここは学習メモとか単なる日報とかを書いていくところ．

記載順は日付の降順

# 20260919

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
1. socketでfdを取得
1. setsockoptでオプションを指定する
1. bindで
1. listenで
1. acceptで
1.

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


## send/write
socket/connectionに書き込む．
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
