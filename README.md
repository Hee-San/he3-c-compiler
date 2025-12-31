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
./he3cc "<入力>" > tmp.s
cc -o tmp tmp.s
./tmp
```

例：

```bash
./he3cc "return 1+2*3;" > tmp.s
cc -o tmp tmp.s
./tmp
echo $?  # 終了コードとして結果が返される
```

## 文法定義 (EBNF)

```
program    ::= stmt*
stmt       ::= "return" expr ";"
             | "if" "(" expr ")" stmt ("else" stmt)?
             | "while" "(" expr ")" stmt
             | "for" "(" expr? ";" expr? ";" expr? ")" stmt
             | "{" stmt* "}"
             | expr ";"
expr       ::= assign
assign     ::= equality ("=" assign)?
equality   ::= relational (("==" | "!=") relational)*
relational ::= add (("<" | "<=" | ">" | ">=") add)*
add        ::= mul (("+" | "-") mul)*
mul        ::= unary (("*" | "/") unary)*
unary      ::= ("+" | "-")? primary
primary    ::= "(" expr ")" | ident args? | num

ident      ::= letter (letter | digit)*
args       ::= "(" ")"
num        ::= digit+
letter     ::= [a-zA-Z_]
digit      ::= [0-9]
```

## テスト

```bash
make test
```

## クリーンアップ

```bash
make clean
```
