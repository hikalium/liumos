#!/usr/bin/env python3
import pexpect
import time
import itertools
import csv
import random
import os
import platform
import datetime
import sys
import json
import parse
import argparse

liumos_root_path = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
print('liumOS root is at: ', liumos_root_path, file=sys.stderr, flush=True)

def launch_qemu():
    p = pexpect.spawn("make -C {} run".format(liumos_root_path))
    p.expect(r"(qemu)")
    print('QEMU Launched', file=sys.stderr, flush=True)
    return p

def connect_to_liumos_serial():
    p = pexpect.spawn("telnet localhost 1235")
    p.expect(r"(liumos)")
    print('Reached to liumos prompt via serial', file=sys.stderr, flush=True)
    return p

def just_print_everything(p): # For debugging
    while True:
        try:
            print(p.readline())
        except pexpect.EOF:
            print("Got EOF.")
            print(p.before)
            sys.exit(1)

def expect_liumos_command_result(p, cmd, expected, timeout):
    p.sendline(cmd);
    try:
        p.expect(expected, timeout=timeout)
    except pexpect.TIMEOUT:
        print("FAIL (timed out): {} => {}".format(cmd, expected))
        print(p.before)
        sys.exit(1)
    print("PASS: {} => {}".format(cmd, expected))


if __name__ == "__main__":
    qemu_mon_conn = launch_qemu()
    time.sleep(1)
    liumos_serial_conn = connect_to_liumos_serial()
    time.sleep(1)
    expect_liumos_command_result(liumos_serial_conn, "ip", "10.0.2.15 eth 52:54:00:12:34:56 mask 255.255.255.0 gateway 10.0.2.2", 5)
    sys.exit(0)

