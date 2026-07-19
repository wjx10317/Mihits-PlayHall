# 内网 Nginx 样例部署（阶段 2）

## 目录约定

将本目录下 `games/` 整棵拷到 Nginx 静态根，例如：

```text
/var/www/games/FooGame/
  manifest.json
  FooGame.exe      # 样例为占位文本，上线请换成真实可执行文件并重算 MD5
  FooGame.dll
  res/demo.txt
```

URL 示例：

- `http://<vm-ip>/games/FooGame/manifest.json`
- `http://<vm-ip>/games/FooGame/FooGame.exe`

与 `t_game_pkg.manifest_url`、`manifest.baseUrl` 保持一致（改 IP 后同步改 MySQL 与 manifest）。

## 最小 nginx.conf 片段

```nginx
server {
    listen 80;
    server_name _;
    root /var/www;
    location /games/ {
        autoindex off;
        default_type application/octet-stream;
    }
}
```

## 重算清单

在仓库根目录执行：

```powershell
python docs/nginx-sample/gen_manifest.py docs/nginx-sample/games/FooGame http://192.168.1.50/games/FooGame/ 32 FooGame 1.0.0
```

## 验证标准

1. `curl http://<vm-ip>/games/FooGame/manifest.json` 返回 JSON
2. 各 `files[].path` 均可 HTTP GET
3. 文件 MD5 与清单一致
