# 内网 Nginx 部署（外部游戏包）

## 目录

```text
/var/www/games/melody_matrix/
  manifest.json
  melody_matrix.exe
  SDL2.dll
  res/...
```

将仓库中 `docs/nginx-sample/games/melody_matrix/` 整目录拷到 Nginx 静态根下的 `games/`。

## URL

- `http://<vm-ip>/games/melody_matrix/manifest.json`
- 与 MySQL `t_game_pkg.manifest_url`、`manifest.baseUrl` 保持一致

## 最小配置

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

```powershell
python docs/nginx-sample/gen_manifest.py docs/nginx-sample/games/melody_matrix http://<vm-ip>/games/melody_matrix/ 32 melody_matrix 1.0.0
```
