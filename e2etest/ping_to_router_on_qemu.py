#!/usr/bin/env python3
import time
import sys
import test_util

if __name__ == "__main__":
    qemu_mon_conn = test_util.launch_qemu()
    time.sleep(1)
    liumos_serial_conn = test_util.connect_to_liumos_serial()
    time.sleep(1)
    test_util.expect_liumos_command_result(liumos_serial_conn, "ping.bin 10.0.2.2", "ICMP packet received from 10.0.2.2 ICMP Type = 0", 5)
    sys.exit(0)

