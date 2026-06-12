# Mihits

基于 Qt/C++ 的在线五子棋对战平台，支持联机对弈、人机对战和对局回放。

## 功能特性

- **用户系统** — 注册 / 登录，MD5 加密传输
- **联机对战** — 基于 UDP 的网络通信，支持房间创建、加入、准备、开始
- **人机对战** — 支持 AI 托管代打
- **对局记录** — 查看历史对局列表，支持单局回放
- **游戏大厅** — 多房间展示，实时显示房间状态与玩家信息

## 项目结构

```
Mihits/
├── main.cpp              # 程序入口
├── ckernel.cpp/h         # 核心调度器（单例），管理网络与界面交互
├── dialog.cpp/h/ui       # 游戏大厅主界面
├── logindialog.cpp/h/ui  # 登录 / 注册界面
├── fiveinlinezone.cpp/h/ui # 五子棋分区（房间列表）
├── roomdialog.cpp/h/ui   # 对战房间界面
├── roomitem.cpp/h/ui     # 房间卡片组件
├── record.cpp/h/ui       # 单条对局记录
├── recordlist.cpp/h/ui   # 对局记录列表
├── fiveinline/           # 五子棋逻辑模块
├── netapi/               # 网络通信模块（UDP）
├── md5/                  # MD5 加密模块
├── bq/                   # 棋子图片资源
├── icon/                 # 头像与背景资源
├── images/               # UI 图标资源
├── tx/                   # 头像图片资源
├── face/                 # 表情图片资源
├── chesscards/           # 棋牌游戏封面
├── vip/                  # VIP 图标资源
└── resource.qrc          # Qt 资源文件
```

## 构建依赖

- Qt 5.x（core gui widgets）
- C++11
- Windows / Linux

## 构建方式

1. 使用 Qt Creator 打开 `Mihits.pro`
2. 配置编译套件（Kit）
3. 点击构建并运行

或使用命令行：

```bash
qmake Mihits.pro
make
```

## 网络配置

服务器 IP 地址在 `ckernel.cpp` 的 `ConfigSet()` 中配置，默认需根据实际服务器地址修改。
