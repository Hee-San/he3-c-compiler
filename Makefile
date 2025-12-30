.PHONY: all clean test help

# コンパイラの設定
CC = gcc
CFLAGS = -std=c11 -g -Wall -Wextra
TARGET = he3cc

# ソースファイルとオブジェクトファイル
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

# デフォルトターゲット
all: $(TARGET)

# コンパイラのビルド
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# オブジェクトファイルのビルド
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# クリーンアップ
clean:
	rm -f $(TARGET) $(OBJS) *.o
	rm -f tmp*

# テストの実行
test: $(TARGET)
	./test.sh

# ヘルプ
help:
	@echo "使用可能なターゲット:"
	@echo "  make       - コンパイラをビルド"
	@echo "  make clean - ビルド成果物をクリーンアップ"
	@echo "  make test  - テストを実行"
	@echo "  make help  - このヘルプを表示"
