extern crate rexpect;

use rexpect::errors::*;
use rexpect::session::PtySession;
use rexpect::spawn;
use std::path::Path;

const TIMEOUT_MILLISECONDS: u64 = 300_000;

fn get_liumos_root_path(exec_path_str: &Path) -> String {
    let exec_path = Path::new(&exec_path_str);
    let mut ancestors = exec_path.ancestors();
    // Take parent of "/path/to/liumos/e2etest/target/debug/e2etest" 4 times here
    // to get liumos_root_dir
    for _ in 0..4 {
        ancestors.next();
    }
    let liumos_root_dir = ancestors.next();
    if liumos_root_dir.is_none() {
        panic!("Failed to determine liumos_root_dir.");
    }
    let liumos_root_dir = String::from(liumos_root_dir.unwrap().to_str().unwrap());
    liumos_root_dir
}

fn launch_liumos(liumos_root_dir: &str) -> PtySession {
    let cmd = format!("make -C {} run_for_e2e_test", liumos_root_dir);
    let mut p = spawn(&cmd, Some(TIMEOUT_MILLISECONDS))
        .unwrap_or_else(|e| panic!("Failed to launch QEMU: {}", e));
    p.exp_regex("\\(qemu\\)").unwrap();
    println!("QEMU launched");
    p
}

fn launch_liumos_on_docker(liumos_root_dir: &str) -> PtySession {
    let cmd = format!("make -C {} run_docker", liumos_root_dir);
    let mut p = spawn(&cmd, Some(TIMEOUT_MILLISECONDS))
        .unwrap_or_else(|e| panic!("Failed to launch QEMU in Docker: {}", e));
    p.exp_regex("\\(qemu\\)").unwrap();
    println!("QEMU launched on Docker");
    p
}

fn connect_to_liumos_serial() -> PtySession {
    let cmd = "telnet localhost 1235";
    let mut p = spawn(&cmd, Some(TIMEOUT_MILLISECONDS))
        .unwrap_or_else(|e| panic!("Failed to connect to liumOS serial: {}", e));
    p.exp_regex("\\(liumos\\)").unwrap();
    println!("Reached to liumOS prompt via serial");
    p
}

fn expect_liumos_command_result(liumos_serial_conn: &mut PtySession, input: &str, expected: &str) {
    liumos_serial_conn.send_line(input).unwrap();
    println!("Sent: {}", input);
    liumos_serial_conn
        .exp_regex(expected)
        .unwrap_or_else(|e| panic!("FAIL: {}\nexpected: {}", e, expected));
    println!("PASS: {}", expected);
}

fn run_end_to_end_tests() -> Result<()> {
    let exec_path_str = std::env::current_exe().unwrap();
    let liumos_root_dir = get_liumos_root_path(&exec_path_str);
    println!("liumOS root dir: {}", liumos_root_dir);

    let mut qemu_mon_conn = match std::env::var("LIUMOS_RUN_TEST_WITHOUT_DOCKER") {
        Ok(_) => {
            std::process::Command::new("make")
                .args(&["stop_docker"])
                .output()
                .unwrap();
            println!("LIUMOS_RUN_TEST_WITHOUT_DOCKER is specified. Running tests without Docker.");
            launch_liumos(&liumos_root_dir)
        }
        Err(_) => launch_liumos_on_docker(&liumos_root_dir),
    };

    qemu_mon_conn.send_line("info version")?;
    qemu_mon_conn.exp_regex("version").unwrap();
    qemu_mon_conn.read_line().unwrap();
    println!(
        "QEMU version: {}",
        qemu_mon_conn.read_line().unwrap().trim()
    );

    let mut liumos_serial_conn = connect_to_liumos_serial();
    liumos_serial_conn.send_line("version")?;
    liumos_serial_conn.exp_regex("liumOS version: ").unwrap();
    println!(
        "liumOS version: {}",
        liumos_serial_conn.read_line().unwrap().trim()
    );

    expect_liumos_command_result(
        &mut liumos_serial_conn,
        "ip",
        "10.0.2.15 eth 52:54:00:12:34:56 mask 255.255.255.0 gateway 10.0.2.2",
    );

    expect_liumos_command_result(
        &mut liumos_serial_conn,
        "ping.bin 10.0.2.2",
        "ICMP packet received from 10.0.2.2 ICMP Type = 0",
    );

    Ok(())
}

fn main() {
    run_end_to_end_tests().unwrap_or_else(|e| panic!("ftp job failed with {}", e));
}
