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
        liumos_builder_conn,
        "/liumos/app/udpserver/udpserver.bin 8888",
        "Listening port: 8888", 5);
    test_util.expect_liumos_command_result(
        liumos_serial_conn,
        "udpclient.bin 10.0.2.2 8888 LIUMOS_E2E_TEST_MESSAGE",
        "Sent size: 24", 5);
    test_util.expect_liumos_command_result(
        liumos_builder_conn,
        "",
        "LIUMOS_E2E_TEST_MESSAGE", 5);
    sys.exit(0)

