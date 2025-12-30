.PHONY: all clean test run help

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

# コンパイラの実行（引数が必要な場合）
run: $(TARGET)
	./$(TARGET) "$(filter-out $@,$(MAKECMDGOALS))"

# make run の後の引数を無視するためのダミーターゲット
%:
	@:

# ヘルプ
help:
	@echo "使用可能なターゲット:"
	@echo "  make       - コンパイラをビルド"
	@echo "  make clean - ビルド成果物をクリーンアップ"
	@echo "  make test  - テストを実行"
	@echo "  make run   - コンパイラを実行"
	@echo "  make help  - このヘルプを表示"
