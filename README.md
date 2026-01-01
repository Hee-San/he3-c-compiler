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
program       ::= function*
function      ::= basetype ident "(" func-params? ")" "{" stmt* "}"

basetype      ::= "int" "*"*
type-suffix   ::= ("[" num "]")*

func-params   ::= func-param ("," func-param)*
func-param    ::= basetype ident type-suffix

stmt          ::= "return" expr ";"
                | "if" "(" expr ")" stmt ("else" stmt)?
                | "while" "(" expr ")" stmt
                | "for" "(" expr? ";" expr? ";" expr? ")" stmt
                | "{" stmt* "}"
                | declaration
                | expr ";"

declaration   ::= basetype ident type-suffix ("=" expr)? ";"

expr          ::= assign
assign        ::= equality ("=" assign)?
equality      ::= relational (("==" | "!=") relational)*
relational    ::= add (("<" | "<=" | ">" | ">=") add)*
add           ::= mul (("+" | "-") mul)*
mul           ::= unary (("*" | "/") unary)*
unary         ::= ("+" | "-" | "*" | "&")? unary
                | postfix

postfix       ::= primary ("[" expr "]")*

primary       ::= "(" expr ")" | ident func-args? | num

func-args     ::= "(" (assign ("," assign)*)? ")"
```

## テスト

```bash
make test
```

## クリーンアップ

```bash
make clean
```
