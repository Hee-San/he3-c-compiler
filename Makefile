.PHONY: all clean test help

# コンパイラの設定
CC = gcc
CFLAGS = -std=c11 -g -static
TARGET = he3cc

# ソースファイルとオブジェクトファイル
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

# デフォルトターゲット
all: $(TARGET)

# コンパイラのビルド
$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

# 全てのオブジェクトファイルはhe3cc.hに依存
$(OBJS): he3cc.h

# オブジェクトファイルのビルド
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# クリーンアップ
clean:
	rm -f $(TARGET) *.o *~ tmp*

# テストの実行
test: $(TARGET)
	./he3cc tests > tmp.s
	gcc -static -o tmp tmp.s
	./tmp

# ヘルプ
help:
	@echo "使用可能なターゲット:"
	@echo "  make       - コンパイラをビルド"
	@echo "  make clean - ビルド成果物をクリーンアップ"
	@echo "  make test  - テストを実行"
	@echo "  make help  - このヘルプを表示"
