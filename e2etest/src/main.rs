extern crate rexpect;

use clap::App;
use clap::Arg;
use rexpect::errors::*;
use rexpect::session::PtySession;
use rexpect::spawn;
use rexpect::ReadUntil;
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

fn connect_to_liumos_builder() -> PtySession {
    let cmd = "docker exec -it liumos-builder0 /bin/bash";
    let mut p = spawn(&cmd, Some(TIMEOUT_MILLISECONDS))
        .unwrap_or_else(|e| panic!("Failed to connect to liumOS builder: {}", e));
    p.exp_regex("\\(liumos-builder\\)").unwrap();
    println!("Reached to liumOS builder shell on Docker");
    p
}

fn connect_to_local_shell() -> PtySession {
    let cmd = "bash";
    let p = spawn(&cmd, Some(TIMEOUT_MILLISECONDS))
        .unwrap_or_else(|e| panic!("Failed to connect to local shell: {}", e));
    println!("Connected to local shell");
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
    let matches = App::new("e2etest")
        .version(env!("CARGO_PKG_VERSION"))
        .about("liumOS End-to-end test runner")
        .arg(
            Arg::new("use-docker")
                .long("use-docker")
                .required(false)
                .takes_value(false)
                .about("Run tests with Docker (mainly for macOS hosts)"),
        )
        .get_matches();

    std::process::Command::new("make")
        .args(&["stop_docker"])
        .output()
        .unwrap();

    std::process::Command::new("killall")
        .args(&["qemu-system-x86_64"])
        .output()
        .unwrap();

    let exec_path_str = std::env::current_exe().unwrap();
    let liumos_root_dir = get_liumos_root_path(&exec_path_str);
    println!("liumOS root dir: {}", liumos_root_dir);

    let use_docker = matches.is_present("use-docker");
    let mut qemu_mon_conn = if use_docker {
        launch_liumos_on_docker(&liumos_root_dir)
    } else {
        launch_liumos(&liumos_root_dir)
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

    let mut liumos_builder_conn = if use_docker {
        connect_to_liumos_builder()
    } else {
        connect_to_local_shell()
    };
    liumos_builder_conn.send_line("uname -a")?;
    let (_, result) = liumos_builder_conn
        .exp_any(vec![
            ReadUntil::String("Linux".into()),
            ReadUntil::String("Darwin".into()),
        ])
        .unwrap();
    println!("liumos_builder_conn uname: {}", result);

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

    liumos_builder_conn.send_line("exit")?;
    qemu_mon_conn.send_line("q")?;
    // liumos_serial_conn will be closed automatically on closing qemu_mon_conn

    Ok(())
}

fn main() {
    run_end_to_end_tests().unwrap_or_else(|e| panic!("ftp job failed with {}", e));
}
