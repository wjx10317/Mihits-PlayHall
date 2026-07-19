# 外部游戏嵌入 — 开发进度

| 阶段 | 状态 | 说明 |
|------|------|------|
| 1 协议/表/冒烟 | **完成** | VERSION 协议 + `t_game_pkg` + GameVersionRq；git `1a15d72` |
| 2 Nginx 样例 | **完成** | `docs/nginx-sample/`；git `30c3061` |
| 3 更新器核心 | **完成** | `GamePackageUpdater`；git `261de4b` |
| 4 专区编排 | **完成（本提交）** | 大厅「外部游戏」按钮 → 模态更新 → JoinZone → QProcess → 退出 LeaveZone |
| 5 失败路径 | **部分完成** | 取消、更新失败问旧版、启动失败 LeaveZone；可再补边缘用例 |

## 运维 checklist（联调前）

1. MySQL 执行 `docs/sql/t_game_pkg.sql`（改 manifest_url 为实际 Nginx IP）
2. 拷贝 `docs/nginx-sample/games/` 到 VM Nginx `root`
3. 真实 exe 替换占位文件后运行 `gen_manifest.py` 重算 MD5
4. 重新编译客户端（需 `QT += network`）

## 比对

- `PlayHallServer` 与 `linux_server/PlayHallServer` 关键文件已再次同步（packdef/clogic）
