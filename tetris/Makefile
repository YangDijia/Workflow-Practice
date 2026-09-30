# 图形化版本 Makefile (SDL2)
# 依赖：sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev

CC       = gcc
CFLAGS   = -Wall -O2
SDL_CFLAGS  := $(shell pkg-config --cflags sdl2 SDL2_ttf SDL2_mixer)
SDL_LIBS    := $(shell pkg-config --libs sdl2 SDL2_ttf SDL2_mixer)

BINDIR   = bin
TARGET   = $(BINDIR)/tetris-game
SRC      = src/Tetris-game.c

.PHONY: all run clean check

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ $< $(SDL_LIBS)
	@echo "✓ 已生成 $@"

# 检查依赖是否安装
check:
	@pkg-config --exists sdl2 SDL2_ttf SDL2_mixer && \
		echo "✓ SDL2 依赖齐全" || \
		(echo "✗ 缺少依赖，请运行：sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev"; exit 1)

# 编译并运行（必须在 graphical/ 目录下运行以读取 assets/）
run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
	@echo "✓ 已清理编译产物"
