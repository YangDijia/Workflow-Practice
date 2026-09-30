# 图形化版本 —— 基于 SDL2

俄罗斯方块的图形界面版本，可实际游玩，带背景音乐和音效。

## 依赖

需要 SDL2 及其扩展库：

```bash
sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev
```

| 库 | 用途 |
|---|---|
| `SDL2` | 窗口、渲染、事件循环 |
| `SDL2_ttf` | 字体渲染（分数、提示文字） |
| `SDL2_mixer` | 背景音乐与音效 |

## 目录说明

```
graphical/
├── src/
│   ├── Tetris-game.c          # 主入口（948 行，含 main 函数）
│   └── archive/               # 早期版本与实验代码（不参与构建）
│       ├── imp.c                  # 旧版完整实现（单色方块）
│       ├── window.c               # 更早的窗口版本
│       ├── block.c                # 方块绘制实验
│       └── text.c                 # SDL_ttf 文字渲染实验
├── assets/                    # 音效资源
│   ├── bgm.mp3                    # 背景音乐
│   ├── clearSound.wav             # 消行音效
│   ├── dropSound.wav              # 落下方块音效
│   └── failureSound.wav           # 游戏结束音效
└── bin/                       # 编译产物
```

## 编译与运行

```bash
make          # 编译 → bin/tetris-game
make run      # 编译并运行
make clean    # 清理
```

手动编译等价于：

```bash
gcc src/Tetris-game.c -o bin/tetris-game \
    $(pkg-config --cflags --libs sdl2 SDL2_ttf SDL2_mixer)
```

> ⚠️ `assets/` 中的音效文件是**运行时**读取的，必须在 `graphical/` 目录下运行程序。

## 操作说明

| 按键 | 功能 |
|------|------|
| `←` `→` | 左右移动 |
| `↑` | 旋转 |
| `↓` | 加速下落 |
| `空格` | 直接落底 |
| `r` | 重新开始 |
| `q` | 退出 |

## 难度设置

代码中定义了三个难度（`Tetris-game.c` 顶部）：

```c
#define EASY_MODE 1
#define NORMAL_MODE 2
#define HARD_MODE 3
```

## 实现说明

- 窗口尺寸：`400 × 800`（棋盘）+ `240`（侧边栏），棋盘 `10 列 × 20 行`
- 方块用编号 `1~7` 区分颜色（I/O/T/S/Z/J/L）
- 内置 AI 评估，可自动落子；权重与 OJ 版本一致
- 字体路径为硬编码：`/usr/share/fonts/truetype/ubuntu/UbuntuMono-RI.ttf`
  （如果系统没有该字体，需修改源码中的路径）

## 历史版本说明

`src/archive/` 中保存了图形版的演进过程：

| 文件 | 差异 |
|------|------|
| `imp.c` | 较早版本：方块只有单色，无 `game_loop()` 重开机制，`q` 直接退出 |
| `window.c` | 更早版本：只有 `0/1` 单色，逻辑更简单 |
| `block.c` | 独立实验：验证方块绘制（`1200×800` 网格），非完整游戏 |
| `text.c` | 独立实验：验证 SDL_ttf 居中文字渲染 |

这些文件仅作参考，不参与构建。
