# 外部游戏嵌入 — 开发进度

| 阶段 | 状态 | 说明 |
|------|------|------|
| 1 协议/表/冒烟 | **完成** | `DEF_GAME_VERSION_*`、`GameVersionRq`、`t_game_pkg.sql`、客户端 `slot_gameVersionRs`；已推送 `1a15d72` |
| 2 Nginx 样例 | **完成** | `docs/nginx-sample/`（manifest、占位包、gen_manifest.py、README） |
| 3 更新器核心 | **完成** | `GamePackageUpdater`：manifest diff / MD5 / 下载 / 删多余 / version.json |
| 4 专区编排 | 待做 | 模态进度 + JoinZone + hide + QProcess + LeaveZone |
| 5 失败路径 | 待做 | 取消/旧版询问/启动失败回滚 |

比对：`PlayHallServer` 工作区关键文件与 `linux_server/PlayHallServer` 一致（阶段1时已对齐）。
