# HTTP client

## ファイル構成
- httpclient.c: liumOSで動かすことを目的としたHTTPクライアント。
- httpclient_tcp.c: TCPを使用し、標準ライブラリを使用しないHTTPクライアント。
- httpclient_stdlib.c: 標準ライブラリを使用するHTTPクライアント。

## Linuxの上で動かす

1. `httpserver`ディレクトリにあるサーバを動かす

```
$ pwd
liumos
$ cd app/httpserver/
$ ./httpserver.bin
```

2. このディレクトリにあるクライアントを異なるターミナルのウィンドウで動かす

```
$ pwd
liumos
$ cd app/httpclient/
$ ./httpclient.bin
```

3. それぞれを動かしたウィンドウで結果が表示される

```
$ ./httpserver.bin
GET / HTTP/1.1
```

```
$ ./httpclient.bin
HTTP/1.1 200 OK
Content-Type: text/html; charset=UTF-8

<html>
...

```

## example.comにリクエストを送る

1. `dig`コマンドでターゲットとなるサイトのIPアドレスを知る

```
$ dig example.com
...
;; ANSWER SECTION:
example.com.            5803    IN      A       93.184.216.34
...
```

2. IP, Port, Host, Pathを指定してリクエストを送る

注：ここで使用しているプログラムは`httpclient_tcp.bin`です。

```
$ ./httpclient_tcp.bin -ip 93.184.216.34 -port 80 -host example.com -path /index.html
----- request -----
GET /index.html HTTP/1.1
Host: example.com

----- request -----
HTTP/1.1 200 OK
Age: 218019
Cache-Control: max-age=604800
Content-Type: text/html; charset=UTF-8
Date: ...
...
```
