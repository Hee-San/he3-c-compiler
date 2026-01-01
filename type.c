#include "he3cc.h"

// 整数型を表すTypeを生成
Type* int_type() {
    Type* ty = calloc(1, sizeof(Type));
    ty->kind = TY_INT;
    return ty;
}

// ポインタ型を表すTypeを生成
Type* pointer_to(Type* base) {
    Type* ty = calloc(1, sizeof(Type));
    ty->kind = TY_PTR;
    ty->base = base;
    return ty;
}

void visit(Node* node) {
    if (!node)
        return;

    // 子ノードを再帰的に訪問
    visit(node->lhs);
    visit(node->rhs);
    visit(node->cond);
    visit(node->then);
    visit(node->els);
    visit(node->init);
    visit(node->inc);

    for (Node* n = node->body; n; n = n->next)
        visit(n);
    for (Node* n = node->args; n; n = n->next)
        visit(n);

    // ノードの種類に応じて型情報を付与
    switch (node->kind) {
        // int型のみ
        case ND_MUL:
        case ND_DIV:
        case ND_EQ:
        case ND_NE:
        case ND_LT:
        case ND_LE:
        case ND_FUN_CALL:
        case ND_NUM:
            node->ty = int_type();
            return;

        // 変数の場合、変数の型を継承
        case ND_LOCAL_VAR:
            node->ty = node->var->ty;
            return;

        // 加算: ptr + int | int + int
        case ND_ADD:
            // int + ptr -> ptr + int
            if (node->rhs->ty->kind == TY_PTR) {
                Node* tmp = node->lhs;
                node->lhs = node->rhs;
                node->rhs = tmp;
            }

            // ptr + ptr はエラー
            if (node->rhs->ty->kind == TY_PTR)
                error_tok(node->tok, "ポインタ同士の加算はできません");

            node->ty = node->lhs->ty;
            return;

        // 減算: ptr - int | ptr - ptr
        case ND_SUB:
            if (node->rhs->ty->kind == TY_PTR)
                error_tok(node->tok, "ポインタを引くことはできません");
            node->ty = node->lhs->ty;
            return;

        // 代入: 左辺の型を継承
        case ND_ASSIGN:
            node->ty = node->lhs->ty;
            return;

        // アドレス取得: T -> T*
        case ND_ADDR:
            node->ty = pointer_to(node->lhs->ty);
            return;

        // デリファレンス: T* -> T
        case ND_DEREF:
            if (node->lhs->ty->kind != TY_PTR)
                error_tok(node->tok, "ポインタではない型のデリファレンスはできません");
            node->ty = node->lhs->ty->base;
            return;
    }
}

void add_type(Program* prog) {
    for (Function* fn = prog->fns; fn; fn = fn->next)
        for (Node* node = fn->node; node; node = node->next)
            visit(node);
}
