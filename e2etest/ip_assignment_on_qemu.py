#!/usr/bin/env python3
import time
import sys
import test_util

def ip_assignment_on_qemu(qemu_mon_conn, liumos_serial_conn, liumos_builder_conn):
    test_util.expect_liumos_command_result(
        liumos_serial_conn,
        "ip",
        "10.0.2.15 eth 52:54:00:12:34:56 mask 255.255.255.0 gateway 10.0.2.2", 5)

if __name__ == "__main__":
    test_util.launch_test(ip_assignment_on_qemu);
    sys.exit(0)
