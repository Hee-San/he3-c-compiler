#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// Tokenizer
//

// トークンの種類
typedef enum {
    TK_RESERVED,  // 記号
    TK_NUM,       // 整数トークン
    TK_EOF,       // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
    TokenKind kind;  // トークンの型
    Token* next;     // 次の入力トークン
    int val;         // kindがTK_NUMの場合、その数値
    char* str;       // トークン文字列
    int len;         // トークンの文字数
};

// 現在着目しているトークン
Token* token;

// 入力プログラム
char* user_input;

// エラー箇所を報告する
void error_at(char* loc, char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, "");  // pos個の空白を出力
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// エラーを報告するための関数
void error(char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char* op) {
    if (token->kind != TK_RESERVED || strncmp(token->str, op, token->len) != 0)
        return false;
    token = token->next;
    return true;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char* op) {
    if (token->kind != TK_RESERVED || strncmp(token->str, op, token->len) != 0)
        error_at(token->str, "'%s'ではありません", op);
    token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
    if (token->kind != TK_NUM)
        error_at(token->str, "数ではありません");
    int val = token->val;
    token = token->next;
    return val;
}

bool at_eof() {
    return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurに繋げる
Token* new_token(TokenKind kind, Token* cur, char* str, int len) {
    Token* tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

// 入力文字列pをトークナイズしてそれを返す
Token* tokenize() {
    char* p = user_input;
    Token head;
    head.next = NULL;
    Token* cur = &head;

    while (*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }

        // 1文字の記号
        if (strchr("+-*/()", *p)) {
            cur = new_token(TK_RESERVED, cur, p, 1);
            p++;
            continue;
        }

        // 数字
        if (isdigit(*p)) {
            char* num_start = p;
            long val = strtol(p, &p, 10);
            int len = (int)(p - num_start);
            cur = new_token(TK_NUM, cur, num_start, len);
            cur->val = val;
            continue;
        }

        error_at(p, "トークナイズできません");
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}

//
// Parser
//

// 抽象構文木のノードの種類
typedef enum {
    ND_ADD,  // +
    ND_SUB,  // -
    ND_MUL,  // *
    ND_DIV,  // /
    ND_NUM,  // 整数
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind;  // ノードの型
    Node* lhs;      // 左辺 (left-hand side)
    Node* rhs;      // 右辺 (right-hand side)
    int val;        // kindがND_NUMの場合のみ使う
};

Node* new_node(NodeKind kind, Node* lhs, Node* rhs) {
    Node* node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node* new_node_num(int val) {
    Node* node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

Node* expr();
Node* mul();
Node* unary();
Node* primary();

// expr = mul ("+" mul | "-" mul)*
Node* expr() {
    Node* node = mul();

    for (;;) {
        if (consume("+"))
            node = new_node(ND_ADD, node, mul());
        else if (consume("-"))
            node = new_node(ND_SUB, node, mul());
        else
            return node;
    }
}

// mul = unary ("*" unary | "/" unary)*
Node* mul() {
    Node* node = unary();

    for (;;) {
        if (consume("*"))
            node = new_node(ND_MUL, node, unary());
        else if (consume("/"))
            node = new_node(ND_DIV, node, unary());
        else
            return node;
    }
}

// unary = ("+" | "-")? primary
Node* unary() {
    if (consume("+"))
        return primary();
    if (consume("-"))
        return new_node(ND_SUB, new_node_num(0), primary());
    return primary();
}

// primary = "(" expr ")" | num
Node* primary() {
    if (consume("(")) {
        Node* node = expr();
        expect(")");
        return node;
    }

    return new_node_num(expect_number());
}

//
// Code generator
//

void gen_push(char* register_name) {
    printf("  sub sp, sp, #16\n");              // スタックポインタを16バイト下げる (領域確保)
    printf("  str %s, [sp]\n", register_name);  // スタックに値を保存
}

void gen_pop(char* register_name) {
    printf("  ldr %s, [sp]\n", register_name);  // スタックから値を復元
    printf("  add sp, sp, #16\n");              // スタックポインタを16バイト上げる (領域解放)
}

void gen(Node* node) {
    if (node->kind == ND_NUM) {
        printf("  mov x0,  #%d\n", node->val);
        gen_push("x0");
        return;
    }

    gen(node->lhs);
    gen(node->rhs);
    gen_pop("x1");
    gen_pop("x0");

    switch (node->kind) {
        case ND_NUM:
            // ND_NUMは上で処理済みなのでここには来ない
            break;
        case ND_ADD:
            printf("  add x0, x0, x1\n");
            break;
        case ND_SUB:
            printf("  sub x0, x0, x1\n");
            break;
        case ND_MUL:
            printf("  mul x0, x0, x1\n");
            break;
        case ND_DIV:
            printf("  sdiv x0, x0, x1\n");
            break;
    }

    gen_push("x0");
}

int main(int argc, char** argv) {
    if (argc != 2) {
        error("引数の個数が正しくありません");
        return 1;
    }

    // トークナイズする
    user_input = argv[1];
    token = tokenize();

    // パースする
    Node* node = expr();

    // アセンブリの前半部分を出力
    printf(".globl main\n");
    printf("main:\n");

    // 抽象構文木を下りながらコード生成
    gen(node);

    // スタックトップに計算結果が残っているはずなので、
    // それをx0にロードしてプログラムを終了する
    gen_pop("x0");
    printf("  ret\n");
    return 0;
}
