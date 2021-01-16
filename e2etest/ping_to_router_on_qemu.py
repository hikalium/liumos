#!/usr/bin/env python3
import time
import sys
import test_util

def test_ping_to_router_on_qemu(qemu_mon_conn, liumos_serial_conn, liumos_builder_conn):
    test_util.expect_liumos_command_result(
        liumos_serial_conn,
        "ping.bin 10.0.2.2",
        "ICMP packet received from 10.0.2.2 ICMP Type = 0", 5)


if __name__ == "__main__":
    test_util.launch_test(test_ping_to_router_on_qemu);
    sys.exit(0)
