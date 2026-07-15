# 棋类合集 (Chess-for-EasyX)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

使用 Dev-C++ 和 EasyX 图形库制作的棋类游戏合集，包含五种经典棋类游戏，全部使用 AI 辅助编程实现。

## 游戏列表

| 游戏 | 规则 | 特色功能 |
|------|------|----------|
| **五子棋** (Gomoku) | 15×15 棋盘，黑白双方轮流落子，五子连珠获胜 | 鼠标悬停预览 |
| **围棋** (Go) | 19×19 棋盘，中国规则（贴目 3.75），BFS 区域计目 | 提子判定、劫争检测、禁自杀、虚手终局 |
| **中国象棋** (Chinese Chess) | 10×9 棋盘，帅/仕/相/马/车/炮/兵完整规则 | 蹩马脚、塞象眼、将帅对面检测、将军/将死/困毙判定、悔棋 |
| **国际象棋** (International Chess) | 8×8 棋盘，标准国际象棋规则 | 王车易位、吃过路兵、升变菜单、将军/将死/逼和判定、悔棋 |
| **高仕足球棋** (Football Chess) | 11×15 棋盘，模拟足球对抗 | 11 个位置（守门员/自由人/后卫/前卫/前锋/中锋），详见规则文档 |

![screenshot](https://raw.githubusercontent.com/Sunset10086/Chess-for-EasyX/main/screenshot.png)

## 系统要求

- **操作系统**：Windows 10/11
- **编译器**：MinGW64 (GCC) 或 MSVC
- **图形库**：[EasyX](https://easyx.cn/) — 需提前安装
- **IDE**（可选）：Dev-C++ 5.11

## 编译与运行

### 安装 EasyX

1. 下载 EasyX：https://easyx.cn/
2. 运行安装程序，选择你的编译器版本

### 使用 Dev-C++ 编译

1. 用 Dev-C++ 打开 `棋类合集.dev` 项目文件
2. 点击「编译运行」(F11)

### 使用命令行编译 (MinGW)

```bash
# 编译所有源文件
g++ -std=c++11 -o 棋类合集.exe main.cpp gobang.cpp go.cpp chchess.cpp chess.cpp football.cpp -leasyx -mwindows
```

### 使用 MSVC 编译

EasyX 对 MSVC 的支持请参考：https://docs.easyx.cn/

## 项目结构

```
Chess-for-EasyX/
├── README.md                       # 本文件
├── LICENSE                         # MIT 许可证
├── .gitignore                      # Git 忽略规则
├── main.cpp                        # 主菜单/启动器
├── gobang.cpp                      # 五子棋
├── go.cpp                          # 围棋
├── chchess.cpp                     # 中国象棋
├── chess.cpp                       # 国际象棋
├── football.cpp                    # 高仕足球棋
├── 棋类合集.dev                     # Dev-C++ 项目文件
├── 附件：高仕足球棋规则.doc          # 足球棋规则文档
└── Icons/                          # 游戏图标
    ├── chessboard.ico
    ├── gobang.ico
    ├── go.ico
    ├── chchess.ico
    ├── chess.ico
    └── football.ico
```

## 开发说明

本项目为个人兴趣作品，使用 Dev-C++ 5.11 + EasyX 在 Windows 上开发，AI 辅助编程实现。

所有游戏均实现了对应的完整规则集，包括边角情况处理。每个游戏文件均可独立运行。

## 许可证

MIT License - 详见 [LICENSE](LICENSE) 文件

<img width="1026" height="784" alt="QQ截图20260715142950" src="https://github.com/user-attachments/assets/29ad0608-b2d9-4b00-bc9f-ec5026f3f531" />
<img width="646" height="669" alt="QQ截图20260715142915" src="https://github.com/user-attachments/assets/e72445ec-281b-43b1-beb9-de359ad6a7ac" />
<img width="481" height="572" alt="QQ截图20260715142857" src="https://github.com/user-attachments/assets/c8aa4b2c-b140-4429-ba33-7cbdb42e4487" />
<img width="616" height="699" alt="QQ截图20260715142825" src="https://github.com/user-attachments/assets/3366c914-7589-4c6d-a128-09075c03306e" />
<img width="666" height="689" alt="QQ截图20260715142800" src="https://github.com/user-attachments/assets/c18f63fb-9667-4564-a6a6-83ce1876fcb9" />
<img width="506" height="549" alt="QQ截图20260715142742" src="https://github.com/user-attachments/assets/1263175e-994a-4061-8155-c7cc3c8132cd" />

