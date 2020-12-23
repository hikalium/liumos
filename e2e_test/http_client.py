#!/usr/bin/env python3
import time
import sys
import test_util

def test_http_client(qemu_mon_conn, liumos_serial_conn, liumos_builder_conn):
    # returns connection to qemu monitor
    test_util.expect_liumos_command_result(
        liumos_builder_conn,
        "app/httpserver/httpserver.bin --port 8888",
        "Listening port: 8888", 5);
    test_util.expect_liumos_command_result(
        liumos_serial_conn,
        "httpclient.bin --ip 10.0.2.2 --port 8888 --path /index.html",
        "HTTP/1.1 200 OK", 5);
    test_util.expect_liumos_command_result(
        liumos_serial_conn,
        "",
        "This is a sample paragraph.", 5);

if __name__ == "__main__":
    test_util.launch_test(test_http_client);
    sys.exit(0)

