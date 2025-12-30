#include "he3cc.h"

void gen_push(char* register_name) {
    printf("  sub sp, sp, #16\n");              // スタックポインタを16バイト下げる (領域確保)
    printf("  str %s, [sp]\n", register_name);  // スタックに値を保存
}

void gen_pop(char* register_name) {
    printf("  ldr %s, [sp]\n", register_name);  // スタックから値を復元
    printf("  add sp, sp, #16\n");              // スタックポインタを16バイト上げる (領域解放)
}

void gen(Node* node) {
    switch (node->kind) {
        case ND_NUM:
            printf("  mov x0,  #%d\n", node->val);
            gen_push("x0");
            return;
        case ND_RETURN:
            gen(node->lhs);
            gen_pop("x0");
            printf("  ret\n");
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
        case ND_EQ:
            printf("  cmp x0, x1\n");
            printf("  cset x0, eq\n");
            break;
        case ND_NE:
            printf("  cmp x0, x1\n");
            printf("  cset x0, ne\n");
            break;
        case ND_LT:
            printf("  cmp x0, x1\n");
            printf("  cset x0, lt\n");
            break;
        case ND_LE:
            printf("  cmp x0, x1\n");
            printf("  cset x0, le\n");
            break;
        case ND_GT:
            printf("  cmp x0, x1\n");
            printf("  cset x0, gt\n");
            break;
        case ND_GE:
            printf("  cmp x0, x1\n");
            printf("  cset x0, ge\n");
            break;
    }

    gen_push("x0");
}

void codegen() {
    // アセンブリの前半部分を出力
    printf(".globl main\n");
    printf("main:\n");

    // 各stmtのコードを生成
    for (int i = 0; code[i]; i++) {
        gen(code[i]);
        gen_pop("x0");  // 式の結果をスタックから捨てる
    }

    // 最後の式の結果がx0に残っているのでそれを返す
    printf("  ret\n");
}
