@echo off
REM 监视 PlayHallServer 源码改动，自动覆盖到 VM（可选编译/重启）
setlocal
cd /d "%~dp0\.."

if not exist "tools\sync_playhall_server.config.json" (
  if exist "tools\sync_playhall_server.config.json.example" (
    echo [提示] 请先复制 tools\sync_playhall_server.config.json.example
    echo        为 tools\sync_playhall_server.config.json 并填写 password
    exit /b 1
  )
)

REM 也可不写配置文件，改用与 MCP 相同的环境变量:
REM   set MIHITS_SSH_HOST=192.168.215.132
REM   set MIHITS_SSH_USER=mihits
REM   set MIHITS_SSH_PASSWORD=...

python tools\sync_playhall_server.py --watch %*
