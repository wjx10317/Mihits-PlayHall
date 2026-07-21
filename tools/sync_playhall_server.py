#!/usr/bin/env python3
"""
PlayHallServer 源码同步到 Linux VM：覆盖远程文件，可选编译/重启。

用法:
  python tools/sync_playhall_server.py              # 全量覆盖一次
  python tools/sync_playhall_server.py --rebuild    # 覆盖 + make
  python tools/sync_playhall_server.py --restart    # 覆盖后重启进程
  python tools/sync_playhall_server.py --watch      # 监视本地改动，自动覆盖
  python tools/sync_playhall_server.py --watch --rebuild --restart

配置优先级: 命令行 > 环境变量 MIHITS_* > tools/sync_playhall_server.config.json
"""
from __future__ import annotations

import argparse
import json
import os
import sys
import time
from pathlib import Path

try:
    import paramiko
except ImportError:
    print("需要 paramiko: pip install paramiko", file=sys.stderr)
    sys.exit(1)

ROOT = Path(__file__).resolve().parents[1]
LOCAL_SERVER = ROOT / "PlayHallServer"
CONFIG_PATH = Path(__file__).resolve().parent / "sync_playhall_server.config.json"

DEFAULTS = {
    "host": "192.168.215.132",
    "port": 22,
    "user": "mihits",
    "password": "",
    "remote_src": "/home/mihits/PlayHall/PlayHallServer",
    "remote_build": "/home/mihits/PlayHall/build-PlayHallServer-Desktop_Qt_5_9_9_GCC_64bit-Debug",
    "ld_library_path": "/home/mihits/Qt5.9.9/5.9.9/gcc_64/lib",
    "server_port": 8000,
    "watch_debounce_sec": 1.0,
}

# 参与同步的相对路径（相对 PlayHallServer/）
SYNC_GLOBS = (
    "include/*.h",
    "src/*.cpp",
    "src/*.h",
    "PlayHallServer.pro",
)


def load_config() -> dict:
    cfg = dict(DEFAULTS)
    if CONFIG_PATH.is_file():
        with open(CONFIG_PATH, encoding="utf-8") as f:
            cfg.update({k: v for k, v in json.load(f).items() if v is not None})
    env_map = {
        "host": "MIHITS_SSH_HOST",
        "port": "MIHITS_SSH_PORT",
        "user": "MIHITS_SSH_USER",
        "password": "MIHITS_SSH_PASSWORD",
        "remote_build": "MIHITS_SERVER_PATH",
        "server_port": "MIHITS_SERVER_PORT",
    }
    for key, env in env_map.items():
        val = os.environ.get(env)
        if val:
            cfg[key] = int(val) if key in ("port", "server_port") else val
    # 若只配了 build 目录，源码默认在其同级 PlayHallServer
    if os.environ.get("MIHITS_REMOTE_SRC"):
        cfg["remote_src"] = os.environ["MIHITS_REMOTE_SRC"]
    return cfg


def iter_local_files() -> list[Path]:
    files: list[Path] = []
    for pattern in SYNC_GLOBS:
        files.extend(LOCAL_SERVER.glob(pattern))
    # 去重、只要文件
    uniq = sorted({p.resolve() for p in files if p.is_file()})
    return uniq


def connect(cfg: dict) -> paramiko.SSHClient:
    if not cfg.get("password") and not os.environ.get("MIHITS_SSH_KEY"):
        print(
            "未配置密码：请设置环境变量 MIHITS_SSH_PASSWORD，"
            f"或复制 {CONFIG_PATH.name}.example 为 {CONFIG_PATH.name}",
            file=sys.stderr,
        )
        sys.exit(2)
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    kwargs = {
        "hostname": cfg["host"],
        "port": int(cfg["port"]),
        "username": cfg["user"],
        "timeout": 15,
    }
    key = os.environ.get("MIHITS_SSH_KEY")
    if key:
        kwargs["key_filename"] = key
    else:
        kwargs["password"] = cfg["password"]
    client.connect(**kwargs)
    return client


def run_ssh(client: paramiko.SSHClient, cmd: str, timeout: int = 180) -> tuple[int, str, str]:
    _, stdout, stderr = client.exec_command(cmd, timeout=timeout)
    out = stdout.read().decode("utf-8", "replace")
    err = stderr.read().decode("utf-8", "replace")
    code = stdout.channel.recv_exit_status()
    return code, out, err


def sync_files(client: paramiko.SSHClient, cfg: dict, only: list[Path] | None = None) -> list[str]:
    remote_src = cfg["remote_src"].rstrip("/")
    files = only if only is not None else iter_local_files()
    if not files:
        raise FileNotFoundError(f"本地无源码可同步: {LOCAL_SERVER}")

    sftp = client.open_sftp()
    uploaded: list[str] = []
    try:
        for local in files:
            rel = local.relative_to(LOCAL_SERVER).as_posix()
            remote = f"{remote_src}/{rel}"
            remote_dir = remote.rsplit("/", 1)[0]
            run_ssh(client, f"mkdir -p '{remote_dir}'")
            sftp.put(str(local), remote)
            uploaded.append(rel)
            print(f"  cover  {rel} -> {remote}")
    finally:
        sftp.close()
    return uploaded


def rebuild(client: paramiko.SSHClient, cfg: dict) -> None:
    build = cfg["remote_build"]
    print(f">>> make in {build}")
    code, out, err = run_ssh(client, f"cd '{build}' && make -j$(nproc) 2>&1", timeout=300)
    text = (out or err).strip()
    if text:
        print(text[-3000:])
    if code != 0:
        raise RuntimeError(f"make failed, exit={code}")


def restart(client: paramiko.SSHClient, cfg: dict) -> None:
    build = cfg["remote_build"]
    ld = cfg["ld_library_path"]
    port = cfg["server_port"]
    print(">>> restart PlayHallServer")
    run_ssh(client, "pkill -f PlayHallServer 2>/dev/null || true; sleep 1")
    start = (
        f"cd '{build}' && export LD_LIBRARY_PATH={ld}:$LD_LIBRARY_PATH && "
        f"ulimit -n 65535 2>/dev/null; "
        f"nohup ./PlayHallServer {port} > /tmp/playhall.log 2>&1 < /dev/null & "
        f"sleep 2; pidof PlayHallServer || (tail -20 /tmp/playhall.log; exit 1)"
    )
    code, out, err = run_ssh(client, start, timeout=60)
    print((out or err).strip() or f"exit={code}")
    if code != 0:
        raise RuntimeError("restart failed")


def snapshot_mtimes() -> dict[Path, float]:
    return {p: p.stat().st_mtime for p in iter_local_files()}


def watch_loop(cfg: dict, do_rebuild: bool, do_restart: bool) -> None:
    debounce = float(cfg.get("watch_debounce_sec", 1.0))
    print(f"watching {LOCAL_SERVER} (debounce={debounce}s, Ctrl+C 退出)")
    last = snapshot_mtimes()
    client = connect(cfg)
    try:
        while True:
            time.sleep(0.5)
            now = snapshot_mtimes()
            changed = [
                p
                for p, m in now.items()
                if p not in last or m > last.get(p, 0) + 1e-6
            ]
            last = now
            if not changed:
                continue
            print(f"\n[{time.strftime('%H:%M:%S')}] changed: "
                  + ", ".join(p.relative_to(LOCAL_SERVER).as_posix() for p in changed))
            time.sleep(debounce)
            # 防抖后再采一次
            last = snapshot_mtimes()
            try:
                sync_files(client, cfg, only=changed)
                if do_rebuild:
                    rebuild(client, cfg)
                if do_restart:
                    restart(client, cfg)
                print("ok")
            except Exception as e:
                print(f"sync error: {e}", file=sys.stderr)
                try:
                    client.close()
                except Exception:
                    pass
                client = connect(cfg)
    except KeyboardInterrupt:
        print("\nstopped")
    finally:
        client.close()


def deploy_once(cfg: dict, do_rebuild: bool, do_restart: bool) -> dict:
    if not LOCAL_SERVER.is_dir():
        raise FileNotFoundError(LOCAL_SERVER)
    client = connect(cfg)
    try:
        uploaded = sync_files(client, cfg)
        if do_rebuild:
            rebuild(client, cfg)
        if do_restart:
            restart(client, cfg)
        return {
            "ok": True,
            "uploaded": uploaded,
            "count": len(uploaded),
            "remote_src": cfg["remote_src"],
            "rebuild": do_rebuild,
            "restart": do_restart,
        }
    finally:
        client.close()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="同步 PlayHallServer 源码到 VM 并可选编译重启")
    parser.add_argument("--watch", action="store_true", help="监视本地改动并自动覆盖")
    parser.add_argument("--rebuild", action="store_true", help="覆盖后在远程 make")
    parser.add_argument("--restart", action="store_true", help="覆盖后重启 PlayHallServer")
    parser.add_argument("--json", action="store_true", help="结果以 JSON 输出（非 watch）")
    args = parser.parse_args(argv)

    cfg = load_config()
    print(f"local : {LOCAL_SERVER}")
    print(f"remote: {cfg['user']}@{cfg['host']}:{cfg['remote_src']}")

    if args.watch:
        watch_loop(cfg, args.rebuild, args.restart)
        return 0

    result = deploy_once(cfg, args.rebuild, args.restart)
    if args.json:
        print(json.dumps(result, ensure_ascii=False, indent=2))
    else:
        print(f"deploy ok, {result['count']} files")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as e:
        print(f"ERROR: {e}", file=sys.stderr)
        sys.exit(1)
