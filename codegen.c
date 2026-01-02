#include "he3cc.h"

// 制御構文でジャンプするためのラベルの通し番号
int labelseq = 0;

// 現在コード生成中の関数名
char *func_name;

// 引数を格納するレジスタの名前
char *argreg1[] = {"w0", "w1", "w2", "w3", "w4", "w5", "w6", "w7"};
char *argreg8[] = {"x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7"};

void gen(Node *node);

void gen_push(char *register_name) {
  printf("  str %s, [sp, -16]!\n", register_name); // sp -= 16; *sp = x0;
}

void gen_pop(char *register_name) {
  printf("  ldr %s, [sp], #16\n", register_name); // x0 = *sp; sp += 16;
}

// ノードのアドレスをx0に格納し、スタックにpushする
void gen_addr(Node *node) {
  switch (node->kind) {
  case ND_VAR: {
    Var *var = node->var;
    if (var->is_local) {
      // ローカル変数:
      // スタック上に配置されるため、フレームポインタ(x29)からの相対オフセットで参照
      int offset = node->var->offset;
      printf("  sub x0, x29, #%d\n", offset);
    } else {
      // グローバル変数:
      // データセクションに配置されるため、ラベル経由でアドレスを取得
      // ARM64の即値制限(12bit)により、64bitアドレスは2命令で構築:
      //   1. adrp: 上位ビット（4KBページアドレス）
      //   2. add :lo12:: 下位12bit（ページ内オフセット）
      printf("  adrp x0, .L.%s\n", var->name);
      printf("  add x0, x0, :lo12:.L.%s\n", var->name);
    }
    gen_push("x0");
    return;
  }
  case ND_DEREF:
    // *ptr: ptrの値がアドレスそのもの
    gen(node->lhs);
    return;
  default:
    break;
  }

  error_tok(node->tok, "代入の左辺値が変数ではありません");
}

void gen_lval(Node *node) {
  if (node->ty->kind == TY_ARRAY) {
    error_tok(node->tok, "配列は代入の左辺値になれません");
  }
  gen_addr(node);
}

// スタックトップにあるアドレスから値をロードして、値をスタックにプッシュする
void load(Type *ty) {
  gen_pop("x0"); // スタックからアドレスを取り出してx0にロード
  if (size_of(ty) == 1) {
    printf("  ldrsb w0, [x0]\n"); // 1バイトレジスタ
  } else {
    printf("  ldr x0, [x0]\n"); // 8バイトレジスタ
  }
  gen_push("x0"); // x0をスタックにプッシュ
}

// スタックトップにある値をスタック2番目にあるアドレスにストアして、値を再びスタックにプッシュする
void store(Type *ty) {
  gen_pop("x1"); // スタックから値を取り出してx1にロード
  gen_pop("x0"); // スタックからアドレスを取り出してx0にロード

  if (size_of(ty) == 1) {
    printf("  strb w1, [x0]\n"); // 1バイトストア
  } else {
    printf("  str x1, [x0]\n"); // 8バイトストア
  }
  gen_push("x1"); // x1をスタックにプッシュ
}

void gen(Node *node) {
  switch (node->kind) {
  case ND_NULL:
    return;
  case ND_NUM:
    printf("  mov x0,  #%d\n", node->val);
    gen_push("x0");
    return;
  case ND_EXPR_STMT:
    gen(node->lhs);
    // 式文は結果を返さない
    printf(
        "  add sp, sp, #16\n"); // スタックポインタを16バイト上げる (領域解放)
    return;
  case ND_VAR:
    gen_addr(node);
    if (node->ty->kind != TY_ARRAY) {
      load(node->ty);
    }
    return;
  case ND_ASSIGN:
    gen_lval(node->lhs);
    gen(node->rhs);
    store(node->ty);
    return;
  case ND_FUN_CALL: {
    // 現状、引数は8個まで対応
    int num_args = 0;
    for (Node *arg = node->args; arg; arg = arg->next) {
      gen(arg);
      num_args++;
    }

    // スタックから逆順でポップして引数レジスタにセット
    for (int i = num_args - 1; i >= 0; i--) {
      gen_pop(argreg8[i]);
    }

    // 関数呼び出し
    printf("  bl %s\n", node->func_name);

    // 戻り値をスタックにプッシュ
    gen_push("x0");
    return;
  }
  case ND_RETURN:
    gen(node->lhs);
    gen_pop("x0");
    printf("  b .L.return.%s\n", func_name);
    return;
  case ND_BLOCK:
  case ND_STMT_EXPR:
    for (Node *n = node->body; n; n = n->next) {
      gen(n);
    }
    return;
  case ND_ADDR:
    gen_addr(node->lhs);
    return;
  case ND_DEREF:
    gen(node->lhs);
    if (node->ty->kind != TY_ARRAY) {
      load(node->ty);
    }
    return;
  case ND_IF: {
    int seq = labelseq++;

    // 条件式
    gen(node->cond);
    gen_pop("x0");
    printf("  cmp x0, #0\n"); // x0と0を比較

    // 条件の結果が0(false)なら
    if (node->els) {
      printf("  b.eq .L.if.else.%d\n", seq); // else節へジャンプ
    } else {
      printf("  b.eq .L.if.end.%d\n", seq); // end節へジャンプ
    }

    // then節
    gen(node->then);
    printf("  b .L.if.end.%d\n", seq); // end節へジャンプ

    if (node->els) {
      // else節
      printf(".L.if.else.%d:\n", seq);
      gen(node->els);
    }

    // end節
    printf(".L.if.end.%d:\n", seq);

    return;
  }
  case ND_WHILE: {
    int seq = labelseq++;

    // 繰り返しの開始ラベル
    printf(".L.while.begin.%d:\n", seq);

    // 条件式
    gen(node->cond);
    gen_pop("x0");
    printf("  cmp x0, #0\n"); // x0と0を比較

    // 条件の結果が0(false)なら繰り返し終了
    printf("  b.eq .L.while.end.%d\n", seq);

    // 繰り返し本体
    gen(node->then);

    // 繰り返しの先頭に戻る
    printf("  b .L.while.begin.%d\n", seq);

    // 繰り返しの終了ラベル
    printf(".L.while.end.%d:\n", seq);

    return;
  }
  case ND_FOR: {
    int seq = labelseq++;

    // 初期化文
    if (node->init)
      gen(node->init);

    // 繰り返しの開始ラベル
    printf(".L.for.begin.%d:\n", seq);

    // 条件式
    if (node->cond) {
      gen(node->cond);
      gen_pop("x0");
      printf("  cmp x0, #0\n"); // x0と0を比較

      // 条件の結果が0(false)なら繰り返し終了
      printf("  b.eq .L.for.end.%d\n", seq);
    }

    // 繰り返し本体
    gen(node->then);

    // 増分文
    if (node->inc)
      gen(node->inc);

    // 繰り返しの先頭に戻る
    printf("  b .L.for.begin.%d\n", seq);

    // 繰り返しの終了ラベル
    printf(".L.for.end.%d:\n", seq);

    return;
  }
  default:
    break;
  }

  gen(node->lhs);
  gen(node->rhs);
  gen_pop("x1");
  gen_pop("x0");

  switch (node->kind) {
  case ND_ADD:
    if (node->ty->base) {
      // ポインタ型の場合、スケーリングする
      printf("  mov x2, #%d\n", size_of(node->ty->base));
      printf("  mul x1, x1, x2\n");
    }
    printf("  add x0, x0, x1\n");
    break;
  case ND_SUB:
    if (node->ty->base) {
      // ポインタ型の場合、スケーリングする
      printf("  mov x2, #%d\n", size_of(node->ty->base));
      printf("  mul x1, x1, x2\n");
    }
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
  default:
    break;
  }

  gen_push("x0");
}

// dataセクションを出力する
void emit_data(Program *prog) {
  printf(".data\n");
  for (VarList *vl = prog->global_vars; vl; vl = vl->next) {
    Var *var = vl->var;
    printf(".globl .L.%s\n", var->name);
    printf(".L.%s:\n", var->name);
    if (var->contents) {
      // 文字列リテラル
      for (int i = 0; i < var->contents_len; i++)
        printf("  .byte %d\n", var->contents[i]);
    } else {
      // グローバル変数
      printf("  .zero %d\n", size_of(var->ty));
    }
  }
}

// 引数をスタックに保存する
void load_arg(Var *var, int idx) {
  int sz = size_of(var->ty);
  if (sz == 1)
    printf("  strb %s, [x29, #-%d]\n", argreg1[idx], var->offset);
  else
    printf("  str %s, [x29, #-%d]\n", argreg8[idx], var->offset);
}

// textセクションを出力する
void emit_text(Program *prog) {
  printf("  .text\n");

  for (Function *fn = prog->fns; fn; fn = fn->next) {
    printf(".globl %s\n", fn->name);
    printf("%s:\n", fn->name);
    func_name = fn->name;

    // Prologue
    printf("  stp x29, x30, [sp, -16]!\n");
    printf("  mov x29, sp\n");
    printf("  sub sp, sp, #%d\n", fn->local_var_stack_size);

    // 引数をスタックに保存
    int i = 0;
    for (VarList *var_list = fn->params; var_list; var_list = var_list->next) {
      // 引数はすでにレジスタに入っているので、それをスタックに保存する
      Var *var = var_list->var;
      load_arg(var, i++);
    }

    // 各stmtのコードを生成
    for (Node *n = fn->node; n; n = n->next) {
      gen(n);
    }

    // Epilogue
    printf(".L.return.%s:\n", func_name);
    printf("  mov sp, x29\n");
    printf("  ldp x29, x30, [sp], #16\n");
    printf("  ret\n");
  }
}

void codegen(Program *prog) {
  emit_data(prog);
  emit_text(prog);
}
