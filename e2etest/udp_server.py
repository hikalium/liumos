#!/usr/bin/env python3
import time
import sys
import test_util

def udp_server(qemu_mon_conn, liumos_serial_conn, liumos_builder_conn):
    test_util.expect_liumos_command_result(
        liumos_serial_conn,
        "udpserver.bin 8889",
        "Listening port: 8889", 5);
    test_util.expect_liumos_command_result(
        liumos_builder_conn,
        "app/udpclient/udpclient.bin 127.0.0.1 8889 LIUMOS_E2E_TEST_MESSAGE",
        "Sent size: 23", 5);
    test_util.expect_liumos_command_result(
        liumos_serial_conn,
        "",
        "LIUMOS_E2E_TEST_MESSAGE", 5);

if __name__ == "__main__":
    test_util.launch_test(udp_server);
    sys.exit(0)
