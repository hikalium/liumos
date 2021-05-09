#!/usr/bin/env python3

# export LIUMOS_RUN_TEST_WITHOUT_DOCKER=1 to run tests on local environment
import pexpect
import os
import sys
import time

liumos_root_path = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
print('liumOS root is at: ', liumos_root_path, file=sys.stderr, flush=True)

def launch_liumos():
    p = pexpect.spawn("make -C {} run_for_e2e_test".format(liumos_root_path))
    p.expect(r"\(qemu\)")
    print('QEMU Launched', file=sys.stderr, flush=True)
    return p

def launch_liumos_on_docker():
    # returns connection to qemu monitor
    p = pexpect.spawn("make run_docker")
    p.expect(r"\(qemu\)", timeout=60)
    print('Reached to qemu monitor on docker', file=sys.stderr, flush=True)
    return p

def connect_to_liumos_serial(retry = 5):
    p = pexpect.spawn("telnet localhost 1235")
    p.expect(r"\(liumos\)")
    print('Reached to liumos prompt via serial', file=sys.stderr, flush=True)
    return p

def connect_to_liumos_builder():
    p = pexpect.spawn("docker exec -it liumos-builder0 /bin/bash")
    p.expect(r"\(liumos-builder\)")
    print('Reached to liumos-builder on docker', file=sys.stderr, flush=True)
    return p

def connect_to_local_shell():
    p = pexpect.spawn("bash", cwd=liumos_root_path)
    print('Local shell opened', file=sys.stderr, flush=True)
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
        print(p.before.decode("utf-8"))
        sys.exit(1)
    print("PASS: {} => {}".format(cmd, expected))

def launch_test_with_docker(test_body_func):
    print("---- Running", test_body_func.__name__);
    qemu_mon_conn = launch_liumos_on_docker();
    liumos_serial_conn = connect_to_liumos_serial();
    liumos_builder_conn = connect_to_liumos_builder();
    print("Sleeping 10 seconds for stability...");
    time.sleep(10);
    test_body_func(qemu_mon_conn, liumos_serial_conn, liumos_builder_conn);
    print("---- PASS ", test_body_func.__name__);
    try:
        qemu_mon_conn.sendline("q");
        qemu_mon_conn.expect("dummy");
        time.sleep(2);
    except pexpect.EOF:
        print("Docker stopped.")
        return
    print("Error: Docker is still running")
    sys.exit(1)

def launch_test_without_docker(test_body_func):
    print("---- Running", test_body_func.__name__);
    qemu_mon_conn = launch_liumos();
    liumos_serial_conn = connect_to_liumos_serial();
    liumos_builder_conn = connect_to_local_shell();
    print("Sleeping 10 seconds for stability...");
    time.sleep(10);
    test_body_func(qemu_mon_conn, liumos_serial_conn, liumos_builder_conn);
    print("---- PASS ", test_body_func.__name__);
    return

def launch_test(test_body_func):
    if(os.environ.get('LIUMOS_RUN_TEST_WITHOUT_DOCKER') == "1"):
        print("---- LIUMOS_RUN_TEST_WITHOUT_DOCKER is specified, running tests without Docker");
        launch_test_without_docker(test_body_func);
    else:
        launch_test_with_docker(test_body_func);
    sys.exit(0)
