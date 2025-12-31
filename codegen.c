#include "he3cc.h"

// 制御構文でジャンプするためのラベルの通し番号
int labelseq = 0;

void gen_push(char* register_name) {
    printf("  str %s, [sp, -16]!\n", register_name);  // sp -= 16; *sp = x0;
}

void gen_pop(char* register_name) {
    printf("  ldr %s, [sp], #16\n", register_name);  // x0 = *sp; sp += 16;
}

void gen_addr(Node* node) {
    if (node->kind == ND_LOCAL_VAR) {
        int offset = node->var->offset;
        printf("  sub x0, x29, #%d\n", offset);
        gen_push("x0");
        return;
    }

    error("代入の左辺値が変数ではありません");
}

// スタックトップにあるアドレスから値をロードして、値をスタックにプッシュする
void load() {
    gen_pop("x0");               // スタックからアドレスを取り出してx0にロード
    printf("  ldr x0, [x0]\n");  // x0が指すアドレスから値をロード
    gen_push("x0");              // x0をスタックにプッシュ
}

// スタックトップにある値をスタック2番目にあるアドレスにストアして、値を再びスタックにプッシュする
void store() {
    gen_pop("x1");               // スタックから値を取り出してx1にロード
    gen_pop("x0");               // スタックからアドレスを取り出してx0にロード
    printf("  str x1, [x0]\n");  // x0が指すアドレスにx1の値をストア
    gen_push("x1");              // x1をスタックにプッシュ
}

void gen(Node* node) {
    switch (node->kind) {
        case ND_NUM:
            printf("  mov x0,  #%d\n", node->val);
            gen_push("x0");
            return;
        case ND_EXPR_STMT:
            gen(node->lhs);
            // 式文は結果を返さない
            printf("  add sp, sp, #16\n");  // スタックポインタを16バイト上げる (領域解放)
            return;
        case ND_LOCAL_VAR:
            gen_addr(node);
            load();
            return;
        case ND_ASSIGN:
            gen_addr(node->lhs);
            gen(node->rhs);
            store();
            return;
        case ND_RETURN:
            gen(node->lhs);
            gen_pop("x0");
            printf("  b .Lreturn\n");
            return;
        case ND_IF: {
            int seq = labelseq++;

            // 条件式
            gen(node->cond);
            gen_pop("x0");
            printf("  cmp x0, #0\n");  // x0と0を比較

            // 条件の結果が0(false)なら
            if (node->els) {
                printf("  b.eq .Lelse%d\n", seq);  // else節へジャンプ
            } else {
                printf("  b.eq .Lend%d\n", seq);  // end節へジャンプ
            }

            // then節
            gen(node->then);
            printf("  b .Lend%d\n", seq);  // end節へジャンプ

            if (node->els) {
                // else節
                printf(".Lelse%d:\n", seq);
                gen(node->els);
            }

            // end節
            printf(".Lend%d:\n", seq);

            return;
        }
        case ND_WHILE: {
            int seq = labelseq++;

            // 繰り返しの開始ラベル
            printf(".Lbegin%d:\n", seq);

            // 条件式
            gen(node->cond);
            gen_pop("x0");
            printf("  cmp x0, #0\n");  // x0と0を比較

            // 条件の結果が0(false)なら繰り返し終了
            printf("  b.eq .Lend%d\n", seq);

            // 繰り返し本体
            gen(node->then);

            // 繰り返しの先頭に戻る
            printf("  b .Lbegin%d\n", seq);

            // 繰り返しの終了ラベル
            printf(".Lend%d:\n", seq);

            return;
        }
    }

    gen(node->lhs);
    gen(node->rhs);
    gen_pop("x1");
    gen_pop("x0");

    switch (node->kind) {
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

void codegen(Program* prog) {
    // アセンブリの前半部分を出力
    printf(".globl main\n");
    printf("main:\n");

    // Prologue
    // 変数26個分の領域を確保する
    printf("  stp x29, x30, [sp, -16]!\n");  // フレームポインタとリターンアドレスを保存
    printf("  mov x29, sp\n");
    printf("  sub sp, sp, #%d\n", prog->local_var_stack_size);  // ローカル変数用の領域を確保

    // 各stmtのコードを生成
    for (Node* n = prog->node; n; n = n->next) {
        gen(n);
    }

    // returnステートメントがない場合は0を返す
    // (C99以降の仕様: main関数はreturnなしで終了すると暗黙的に0を返す)
    // TODO: 将来、一般的な関数を実装する際は、main関数のみこの処理を行うようにする
    printf("  mov x0, #0\n");

    // Epilogue
    printf("  .Lreturn:\n");
    printf("  mov sp, x29\n");
    printf("  ldp x29, x30, [sp], #16\n");
    printf("  ret\n");
}
