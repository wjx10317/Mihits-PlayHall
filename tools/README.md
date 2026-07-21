# PlayHallServer 同步工具

把本机 `PlayHallServer/` 源码 **覆盖** 到虚拟机，支持监视自动同步。

## 一次覆盖

```powershell
python tools/sync_playhall_server.py
python tools/sync_playhall_server.py --rebuild --restart
```

## 改动即覆盖（自动）

```powershell
tools\watch_playhall_server.bat
# 或
python tools/sync_playhall_server.py --watch --rebuild --restart
```

保存 `PlayHallServer/include|src` 下文件后，约 1 秒内 scp 覆盖到 VM。

## 配置

1. 复制 `sync_playhall_server.config.json.example` → `sync_playhall_server.config.json`（已 gitignore）
2. 或使用与 MCP 相同的环境变量：`MIHITS_SSH_HOST/USER/PASSWORD`、`MIHITS_SERVER_PATH`

## MCP

`mihits-monitor` 增加工具 `deploy_playhall_server`（覆盖 + 默认 make/重启）。需重启 Cursor MCP 后生效。
