#include "he3cc.h"

VarList* local_vars;

// 既存のローカル変数を名前で検索する関数
Var* find_local_var(Token* tok) {
    for (VarList* var_list = local_vars; var_list; var_list = var_list->next) {
        Var* var = var_list->var;
        if (tok->len == strlen(var->name) &&
            strncmp(tok->str, var->name, tok->len) == 0) {
            return var;
        }
    }
    return NULL;
}

// 新しいノードを生成する関数
Node* new_node(NodeKind kind, Token* tok) {
    Node* node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

// 二項演算子ノードを生成する関数
Node* new_node_binary_op(NodeKind kind, Node* lhs, Node* rhs, Token* tok) {
    Node* node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

// 単項演算子ノードを生成する関数
Node* new_node_unary_op(NodeKind kind, Node* operand, Token* tok) {
    Node* node = new_node(kind, tok);
    node->lhs = operand;
    return node;
}

// 数値ノードを生成する関数
Node* new_node_num(int val, Token* tok) {
    Node* node = new_node(ND_NUM, tok);
    node->val = val;
    return node;
}

// ローカル変数ノードを生成する関数
Node* new_local_var(Var* var, Token* tok) {
    Node* node = new_node(ND_LOCAL_VAR, tok);
    node->var = var;
    return node;
}

// 新しいローカル変数をローカル変数リストに追加する関数
Var* push_local_var(Token* tok) {
    Var* var = calloc(1, sizeof(Var));
    var->name = strndup(tok->str, tok->len);

    VarList* var_list = calloc(1, sizeof(VarList));
    var_list->var = var;
    var_list->next = local_vars;

    // オフセットの割り当てはパーサーの役割ではないので、ここでは行わない
    local_vars = var_list;
    return var;
}

Function* function();
Node* stmt();
Node* expr();
Node* assign();
Node* equality();
Node* relational();
Node* add();
Node* mul();
Node* unary();
Node* primary();
Node* func_args();
VarList* func_params();

// program = function*
Program* program() {
    Function head;
    head.next = NULL;
    Function* cur = &head;

    while (!at_eof()) {
        cur->next = function();
        cur = cur->next;
    }

    Program* prog = calloc(1, sizeof(Program));
    prog->fns = head.next;
    return prog;
}

// function = ident "(" func_params? ")" "{" stmt* "}"
Function* function() {
    local_vars = NULL;

    Function* fn = calloc(1, sizeof(Function));
    fn->name = expect_ident();
    expect("(");
    fn->params = func_params();
    expect("{");

    Node head;
    head.next = NULL;
    Node* cur = &head;

    while (!consume("}")) {
        cur->next = stmt();
        cur = cur->next;
    }

    fn->node = head.next;
    fn->local_vars = local_vars;
    return fn;
}

// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "{" stmt* "}"
//      | expr ";"
Node* stmt() {
    Token* tok;
    if (tok = consume("return")) {
        Node* node = new_node_unary_op(ND_RETURN, expr(), tok);
        expect(";");
        return node;
    }

    if (tok = consume("if")) {
        Node* node = new_node(ND_IF, tok);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        if (consume("else")) {
            node->els = stmt();
        }
        return node;
    }

    if (tok = consume("while")) {
        Node* node = new_node(ND_WHILE, tok);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        return node;
    }

    if (tok = consume("for")) {
        Node* node = new_node(ND_FOR, tok);
        expect("(");
        if (!consume(";")) {
            node->init = expr();
            expect(";");
        }
        if (!consume(";")) {
            node->cond = expr();
            expect(";");
        }
        if (!consume(")")) {
            node->inc = expr();
            expect(")");
        }
        node->then = stmt();
        return node;
    }

    if (tok = consume("{")) {
        Node* node = new_node(ND_BLOCK, tok);
        Node head;
        head.next = NULL;
        Node* cur = &head;

        while (!consume("}")) {
            cur->next = stmt();
            cur = cur->next;
        }

        node->body = head.next;
        return node;
    }

    tok = token;
    Node* node = new_node_unary_op(ND_EXPR_STMT, expr(), tok);
    expect(";");
    return node;
}

// expr = assign
Node* expr() {
    return assign();
}

// assign = equality ("=" assign)?
Node* assign() {
    Node* node = equality();
    Token* tok;
    if (tok = consume("="))
        node = new_node_binary_op(ND_ASSIGN, node, assign(), tok);
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
Node* equality() {
    Node* node = relational();
    Token* tok;

    for (;;) {
        if (tok = consume("=="))
            node = new_node_binary_op(ND_EQ, node, relational(), tok);
        else if (tok = consume("!="))
            node = new_node_binary_op(ND_NE, node, relational(), tok);
        else
            return node;
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node* relational() {
    Node* node = add();
    Token* tok;

    for (;;) {
        if (tok = consume("<"))
            node = new_node_binary_op(ND_LT, node, add(), tok);
        else if (tok = consume("<="))
            node = new_node_binary_op(ND_LE, node, add(), tok);
        else if (tok = consume(">"))
            node = new_node_binary_op(ND_GT, node, add(), tok);
        else if (tok = consume(">="))
            node = new_node_binary_op(ND_GE, node, add(), tok);
        else
            return node;
    }
}

// add = mul ("+" mul | "-" mul)*
Node* add() {
    Node* node = mul();
    Token* tok;

    for (;;) {
        if (tok = consume("+"))
            node = new_node_binary_op(ND_ADD, node, mul(), tok);
        else if (tok = consume("-"))
            node = new_node_binary_op(ND_SUB, node, mul(), tok);
        else
            return node;
    }
}

// mul = unary ("*" unary | "/" unary)*
Node* mul() {
    Node* node = unary();
    Token* tok;

    for (;;) {
        if (tok = consume("*"))
            node = new_node_binary_op(ND_MUL, node, unary(), tok);
        else if (tok = consume("/"))
            node = new_node_binary_op(ND_DIV, node, unary(), tok);
        else
            return node;
    }
}

// unary = ("+" | "-")? primary
Node* unary() {
    Token* tok;
    if (tok = consume("+"))
        return primary();
    if (tok = consume("-"))
        return new_node_binary_op(ND_SUB, new_node_num(0, tok), primary(), tok);
    return primary();
}

// primary = "(" expr ")" | ident func-args? | num
Node* primary() {
    if (consume("(")) {
        Node* node = expr();
        expect(")");
        return node;
    }

    Token* tok;
    if (tok = consume_ident()) {
        if (consume("(")) {
            Node* node = new_node(ND_FUN_CALL, tok);
            node->func_name = strndup(tok->str, tok->len);
            node->args = func_args();
            return node;
        }

        Var* var = find_local_var(tok);
        if (!var) {
            var = push_local_var(tok);
        }
        return new_local_var(var, tok);
    }

    tok = token;
    if (tok->kind != TK_NUM)
        error_tok(tok, "式が必要です");

    return new_node_num(expect_number(), tok);
}

// func-params = ident ("," ident)*
VarList* func_params() {
    if (consume(")")) {
        return NULL;
    }

    VarList* head = calloc(1, sizeof(VarList));
    head->var = push_local_var(consume_ident());
    VarList* cur = head;

    while (!consume(")")) {
        expect(",");
        cur->next = calloc(1, sizeof(VarList));
        cur->next->var = push_local_var(consume_ident());
        cur = cur->next;
    }

    return head;
}

// func-args = "(" (assign ("," assign)*)? ")"
Node* func_args() {
    if (consume(")")) {
        return NULL;
    }

    Node* head = assign();
    Node* cur = head;
    while (consume(",")) {
        cur->next = assign();
        cur = cur->next;
    }
    expect(")");
    return head;
}
