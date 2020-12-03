#!/usr/bin/env python3
import time
import sys
import test_util

if __name__ == "__main__":
    qemu_mon_conn = test_util.launch_qemu()
    time.sleep(1)
    liumos_serial_conn = test_util.connect_to_liumos_serial()
    time.sleep(1)
    test_util.expect_liumos_command_result(liumos_serial_conn, "ip", "10.0.2.15 eth 52:54:00:12:34:56 mask 255.255.255.0 gateway 10.0.2.2", 5)
    sys.exit(0)

