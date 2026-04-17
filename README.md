# 貪吃蛇：烈空座進化 (Linux / macOS)

終端機貪吃蛇小遊戲，支援 Linux 與 macOS，包含進化與技能系統。

## 平台支援

- Linux (g++ / clang++)
- macOS (Apple clang++)

## 編譯

```bash
g++ -std=c++11 -O2 -o snake_game_cpp snake_game.cpp
```

## 執行

```bash
./snake_game_cpp
```

## 控制

- 方向鍵：移動
- WASD：移動 (備用)
- E：龍衝（逗逗龍）
- SPACE：龍波（烈空座）
- Q：暴風（烈空座）
- P：暫停

## 存檔路徑

- 預設：`$HOME/.snake_evolution/highscores.txt`
- 可用環境變數覆蓋：`SNAKE_HIGHSCORE_FILE`

例如：

```bash
SNAKE_HIGHSCORE_FILE=./highscores.txt ./snake_game_cpp
```

## macOS 注意事項

- 建議使用 iTerm2 或 Terminal 最新版。
- 若終端機顯示異常，可輸入：

```bash
reset
```
