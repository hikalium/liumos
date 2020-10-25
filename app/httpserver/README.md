# HTTP server 

## Run on localhost

1. Run the server by `./httpserver.bin`.
2. Run the client by `./httpclient.bin` in another terminal.
3. You will see:

```
$ ./httpserver.bin
GET /index.html HTTP/1.1
```

```
$ ./httpclient.bin
HTTP/1.1 200 OK
```

## client: send a request to example.com
```
$ dig example.com
...
;; ANSWER SECTION:
example.com.            5803    IN      A       93.184.216.34
...

$ ./httpclient.bin example.com 93.184.216.34
----- request -----
GET / HTTP/1.1
Host: example.com

----- request -----
HTTP/1.1 200 OK
Age: 218019
Cache-Control: max-age=604800
Content-Type: text/html; charset=UTF-8
Date: ...
...
```
