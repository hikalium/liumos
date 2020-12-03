#!/usr/bin/env python3
import time
import sys
import test_util

if __name__ == "__main__":
    qemu_mon_conn = test_util.launch_liumos_on_docker()
    time.sleep(2)
    liumos_serial_conn = test_util.connect_to_liumos_serial()
    liumos_builder_conn = test_util.connect_to_liumos_builder()
    test_util.expect_liumos_command_result(
        liumos_serial_conn,
        "udpserver.bin 8889",
        "Listening port: 8889", 5);
    test_util.expect_liumos_command_result(
        liumos_builder_conn,
        "/liumos/app/udpclient/udpclient.bin 127.0.0.1 8889 LIUMOS_E2E_TEST_MESSAGE",
        "Sent size: 23", 5);
    test_util.expect_liumos_command_result(
        liumos_serial_conn,
        "",
        "LIUMOS_E2E_TEST_MESSAGE", 5);
    sys.exit(0)

