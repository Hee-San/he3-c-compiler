# he3cc - He3 C Compiler

ARM64アーキテクチャ向けの小さなCコンパイラです。

[低レイヤを知りたい人のためのCコンパイラ作成入門](https://www.sigbus.info/compilerbook)を参考に作成しています。

## 必要な環境

- ARM64 (aarch64) アーキテクチャのLinux環境
- GCC または Clang
- Bash

## ビルド方法

```bash
make
```

または

```bash
cc -o he3cc he3cc.c
```

## 使用方法

```bash
./he3cc <入力> > tmp.s
cc -o tmp tmp.s
./tmp
```

Makefileを使った実行：

```bash
make run 123
```

## テスト

```bash
make test
```

## クリーンアップ

```bash
make clean
```
