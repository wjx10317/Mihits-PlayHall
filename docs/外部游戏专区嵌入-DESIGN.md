# 外部游戏专区嵌入 — 开发设计文档

| 项 | 内容 |
|----|------|
| 状态 | 方案已共识，待实现 |
| 关联 | Mihits 大厅客户端 + PlayHallServer |
| 日期 | 2026-07-19 |

---

## 1. 目标与范围

### 1.1 目标

在现有大厅中增加「外部游戏」专区入口：

1. 以 **独立进程** 运行子游戏（独立游戏服，大厅不代连、不做子游戏登录）。
2. 大厅侧仍视为 **专区**：`JoinZone` / `LeaveZone`，进入后 **隐藏大厅界面**。
3. 支持 **版本校验 + 按清单增量更新**（MD5）；首次或本地为空时 diff 为全集，等价全量。
4. 资源通过 **内网 Nginx** 以 HTTP 提供；版本元数据在 **MySQL**。

### 1.2 非目标（本期不做）

- 子进程登录 / token 换票 / 大厅代转发游戏协议
- 父子进程 IPC（仅监听进程退出）
- 客户端拉源码本地编译
- 二进制差分包（bsdiff 等）
- 公网 CDN / HTTPS 签名 URL（协议预留 URL 字符串即可，部署先内网 HTTP）
- 改造现有同进程五子棋专区为外部进程（五子棋保持现状）

### 1.3 与现有五子棋对比

| | 五子棋（现有） | 外部游戏（本期） |
|--|----------------|------------------|
| UI | 同进程切换 `fiveinlinezone` | 隐藏大厅 + 启动子进程 |
| 网络 | PlayHallServer 房间/落子 | 子游戏自己的服务器 |
| 专区 | `JoinZone` | 同样 `JoinZone` / `LeaveZone` |
| 资源更新 | 无 | manifest + MD5 增量 |

---

## 2. 总体架构

```text
点击外部游戏专区
  → 模态进度：VERSION_RQ(zoneid)  --TCP--> PlayHallServer（读 t_game_pkg）
  → HTTP GET manifest_url
  → 与本地 version.json 做 MD5 diff
       downloadList = 变更或缺失
       deleteList   = 本地多余（清单中已无）
  → 按 diff 拉取文件并校验 → 删多余 → 写 version.json
  → JoinZone(zoneid)
  → 隐藏大厅
  → QProcess 启动 games/{exe_name}/{exe_name}.exe
  → 进程 finished
  → 显示大厅 + LeaveZone
```

```text
客户端                          大厅服                         资源
Qt 大厅 --TCP packdef--------> PlayHallServer + MySQL
       --HTTP GET------------> 内网 VM Nginx（静态目录）
       --QProcess------------> 子游戏进程（独立游戏服）
```

---

## 3. 关键共识一览

| # | 决策 |
|---|------|
| 1 | 独立子进程 + 独立游戏服；大厅无子游戏登录 |
| 2 | 算专区；进入隐藏大厅 |
| 3 | 进/出发 `JoinZone` / `LeaveZone` |
| 4 | 版本问询走大厅 TCP；包体走 HTTP（Nginx） |
| 5 | 退出只监听 `QProcess::finished` → show + LeaveZone |
| 6 | 先更新成功，再 JoinZone → 藏大厅 → 起进程 |
| 7 | `zoneid` 即游戏包主键 |
| 8 | 本地 `games/{exe_name}/version.json` |
| 9 | 启动入口：`games/{exe_name}/{exe_name}.exe` |
| 10 | 未安装必须更新成功；已装更新失败可询问是否用旧版进入 |
| 11 | 元数据 MySQL；含 `exe_name`、**仅一个** `manifest_url` |
| 12 | 全量 = diff 为清单全集；不另设 fullPackUrl |
| 13 | 更新后删除本地多余文件（清单中已不存在的 path） |
| 14 | 增量哈希：**MD5**（使用工程已有 `md5/`） |
| 15 | 包结构主要为：可执行文件、动态库、`res/` 资源；实务上常仅 exe 变更 |
| 16 | 拉包/更新使用 **模态进度框**（取消则不进专区） |
| 17 | HTTP：内网虚拟机 **Nginx** 静态资源 |

---

## 4. 数据与协议设计

### 4.1 MySQL：`t_game_pkg`

```sql
CREATE TABLE t_game_pkg (
  zoneid        INT          NOT NULL PRIMARY KEY COMMENT '与专区 ID 相同',
  version       VARCHAR(32)  NOT NULL COMMENT '当前版本号',
  exe_name      VARCHAR(64)  NOT NULL COMMENT '可执行文件名(无扩展名)，即安装目录名',
  manifest_url  VARCHAR(512) NOT NULL COMMENT '清单唯一入口 URL',
  release_note  VARCHAR(256) NOT NULL DEFAULT '',
  updated_at    TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP
                             ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

说明：

- **不设** `full_url`：全量通过「diff = 全部文件」完成。
- `exe_name` 示例：`FooGame` → 本地与 Nginx 目录均为 `games/FooGame/`，入口 `FooGame.exe`。

### 4.2 大厅 TCP：版本校验（建议包号，实现时写入两端 packdef）

**请求 `STRU_GAME_VERSION_RQ`**

| 字段 | 类型 | 说明 |
|------|------|------|
| type | PackType | 新协议号 |
| userid | int | 当前用户 |
| zoneid | int | 专区 / 游戏 ID |

**响应 `STRU_GAME_VERSION_RS`**

| 字段 | 类型 | 说明 |
|------|------|------|
| type | PackType | |
| result | int | 0=失败（无此游戏等）1=成功 |
| zoneid | int | |
| serverVersion | char[] | 与表 `version` 一致 |
| exe_name | char[] | 与表一致 |
| manifest_url | char[] | 唯一资源 URL |
| release_note | char[] | 可选说明 |
| needUpdate | int | 可选；也可仅由客户端比本地 version |

服务端：按 `zoneid` 查 `t_game_pkg` 填 RS。

### 4.3 HTTP：`manifest.json`（由 Nginx 提供）

```json
{
  "zoneid": 32,
  "version": "1.0.3",
  "exe_name": "FooGame",
  "baseUrl": "http://192.168.x.x/games/FooGame/",
  "files": [
    { "path": "FooGame.exe", "size": 5242880, "md5": "..." },
    { "path": "xxx.dll",     "size": 102400,  "md5": "..." },
    { "path": "res/a.png",   "size": 40960,   "md5": "..." }
  ]
}
```

- `path`：相对 `games/{exe_name}/`。
- 下载 URL：`baseUrl + path`（或等价拼接规则，实现时固定一种）。
- 校验：现有工程 MD5，与清单一致才替换正式文件。

### 4.4 本地 `version.json`

更新成功后写入，内容对齐当前服务端清单的 `version` + `files`（path+md5），供下次 diff。

路径：`games/{exe_name}/version.json`。

---

## 5. 客户端更新算法

```text
1. VERSION_RQ → RS；result!=1 则提示失败，结束
2. installDir = games/{exe_name}/
3. 加载本地 version.json → localMap[path]=md5；无文件则 localMap 为空
4. HTTP GET manifest_url → newManifest
5. downloadList = { f in newManifest.files | localMap[f.path] != f.md5 }
6. deleteList   = { path in localMap | path 不在 newManifest.files }
   （勿把「更新器管理范围外」的路径算入；仅限本 installDir 内由清单管理的文件）
7. 对 downloadList：
     下载到 path.tmp → MD5 校验 → 原子替换为 path
8. 删除 deleteList 中的文件
9. 写 version.json = { version, files: newManifest.files }
10. JoinZone → hide 大厅 → start {installDir}/{exe_name}.exe
```

### 5.1 失败策略

| 情况 | 行为 |
|------|------|
| 从未安装且更新失败/取消 | 不 JoinZone，不起进程 |
| 已安装且更新失败 | 询问是否仍用旧版进入；否同上 |
| 用户取消模态进度 | 同「未成功」，不进专区 |

### 5.2 进程退出

`QProcess::finished` → 显示大厅 → `LeaveZone`。无额外 IPC。

---

## 6. Nginx 部署约定（内网 VM）

示例目录：

```text
/var/www/games/
  FooGame/
    manifest.json
    FooGame.exe
    xxx.dll
    res/
      ...
```

示例配置要点：

- `root` 指向 `/var/www`（或等价），URL：`http://<vm-ip>/games/FooGame/...`
- 放行防火墙 80 端口；客户端需能访问该内网 IP
- `t_game_pkg.manifest_url` = `http://<vm-ip>/games/FooGame/manifest.json`

与 PlayHallServer 可同机或分机；**推荐资源流量与 epoll 业务分离**（独立 VM 更清晰）。

---

## 7. 模块改动面（实现指引）

| 模块 | 改动 |
|------|------|
| `packdef.h`（两端） | 增加 VERSION_RQ/RS |
| `clogic.cpp` | 查 `t_game_pkg`，回 RS |
| MySQL | 建表 + 配置行 |
| 大厅 UI | 外部专区入口按钮/图标 |
| `ckernel` | 编排：版本→下载→JoinZone→hide→QProcess→LeaveZone |
| 新小组件（建议） | `GamePackageUpdater`：manifest diff、MD5、下载、删多余 |
| `md5/` | 复用现有实现 |
| Nginx | 运维侧静态目录 |

客户端 HTTP：优先 `QNetworkAccessManager`（与 Qt 大厅一体）。

---

## 8. 分阶段实现与验证标准

| 阶段 | 内容 | 验证标准 |
|------|------|----------|
| **1 数据与协议** | 建 `t_game_pkg`；两端 packdef；服务端 VERSION 处理 | 客户端能收到正确 version/exe_name/manifest_url |
| **2 Nginx + 样例包** | VM 部署目录与 manifest；至少含 exe+可选 dll/res | 浏览器/curl 能打开 manifest 与 exe URL |
| **3 更新器** | diff / 下载 / MD5 / deleteList / version.json | 首次下全集；改 exe md5 后仅下 exe；删清单外残留文件 |
| **4 专区编排** | 模态进度；成功后 JoinZone+hide+QProcess；退出 LeaveZone+show | 完整点进→玩游戏→关游戏回大厅；取消不进专区 |
| **5 失败路径** | 无网、404、旧版询问 | 符合 §5.1；不残留半下载损坏 exe（tmp 校验失败不替换） |

每阶段通过后再进入下一阶段。

---

## 9. 风险与对策

| 风险 | 对策 |
|------|------|
| 更新中杀进程导致半文件 | 先写 `.tmp`，MD5 通过后再 rename |
| JoinZone 后启动失败 | 启动失败则立即 LeaveZone + show，避免卡在「幽灵专区」 |
| 误删本地文件 | deleteList 仅针对清单 path 集合差；保留 version.json 至最后重写 |
| MD5 碰撞（理论） | 本期可接受；非防篡改方案，仅完整性 |
| 内网 IP 变更 | 只改 MySQL `manifest_url` 与 manifest 内 `baseUrl` |

---

## 10. 编码约定

- 协议结构与现有 `packdef` 风格一致（定长 char 数组、两端同步）。
- 下载与 MD5 放独立类，避免继续膨胀 `ckernel.cpp`。
- 不把大文件经现有大厅 TCP 传输。
- 外部专区 `zoneid` 避开已有 `Five_In_Line = 0x10`。

---

## 11. 文档修订记录

| 日期 | 说明 |
|------|------|
| 2026-07-19 | 初稿：方案质询共识落地；双 URL 合并为 manifest；多余文件删除；Nginx + exe_name 目录 |

---

**门禁：** 本文档确认后，按 §8 分阶段实现；HTTP 细参（并发数、是否 Range 断点）可在阶段 3 内取默认值（串行或小并发、第一期可不做 Range），不阻塞开工。
