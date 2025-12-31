#include "he3cc.h"

LocalVar* local_vars;

// 既存のローカル変数を名前で検索する関数
LocalVar* find_local_var(Token* tok) {
    for (LocalVar* var = local_vars; var; var = var->next) {
        if (tok->len == strlen(var->name) &&
            strncmp(tok->str, var->name, tok->len) == 0) {
            return var;
        }
    }
    return NULL;
}

// 新しいノードを生成する関数
Node* new_node(NodeKind kind) {
    Node* node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

// 二項演算子ノードを生成する関数
Node* new_node_binary_op(NodeKind kind, Node* lhs, Node* rhs) {
    Node* node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

// 単項演算子ノードを生成する関数
Node* new_node_unary_op(NodeKind kind, Node* operand) {
    Node* node = new_node(kind);
    node->lhs = operand;
    return node;
}

// 数値ノードを生成する関数
Node* new_node_num(int val) {
    Node* node = new_node(ND_NUM);
    node->val = val;
    return node;
}

// ローカル変数ノードを生成する関数
Node* new_local_var(LocalVar* var) {
    Node* node = new_node(ND_LOCAL_VAR);
    node->var = var;
    return node;
}

// 新しいローカル変数をローカル変数リストに追加する関数
LocalVar* push_local_var(Token* tok) {
    LocalVar* var = calloc(1, sizeof(LocalVar));
    var->next = local_vars;
    var->name = strndup(tok->str, tok->len);
    // オフセットの割り当てはパーサーの役割ではないので、ここでは行わない
    local_vars = var;
    return var;
}

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

// program = stmt*
Program* program() {
    local_vars = NULL;

    Node head;
    head.next = NULL;
    Node* cur = &head;

    while (!at_eof()) {
        cur->next = stmt();
        cur = cur->next;
    }

    Program* prog = calloc(1, sizeof(Program));
    prog->node = head.next;
    prog->local_vars = local_vars;
    return prog;
}

// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "{" stmt* "}"
//      | expr ";"
Node* stmt() {
    if (consume("return")) {
        Node* node = new_node_unary_op(ND_RETURN, expr());
        expect(";");
        return node;
    }

    if (consume("if")) {
        Node* node = new_node(ND_IF);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        if (consume("else")) {
            node->els = stmt();
        }
        return node;
    }

    if (consume("while")) {
        Node* node = new_node(ND_WHILE);
        expect("(");
        node->cond = expr();
        expect(")");
        node->then = stmt();
        return node;
    }

    if (consume("for")) {
        Node* node = new_node(ND_FOR);
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

    if (consume("{")) {
        Node* node = new_node(ND_BLOCK);
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

    Node* node = new_node_unary_op(ND_EXPR_STMT, expr());
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
    if (consume("="))
        node = new_node_binary_op(ND_ASSIGN, node, assign());
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
Node* equality() {
    Node* node = relational();

    for (;;) {
        if (consume("=="))
            node = new_node_binary_op(ND_EQ, node, relational());
        else if (consume("!="))
            node = new_node_binary_op(ND_NE, node, relational());
        else
            return node;
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node* relational() {
    Node* node = add();

    for (;;) {
        if (consume("<"))
            node = new_node_binary_op(ND_LT, node, add());
        else if (consume("<="))
            node = new_node_binary_op(ND_LE, node, add());
        else if (consume(">"))
            node = new_node_binary_op(ND_GT, node, add());
        else if (consume(">="))
            node = new_node_binary_op(ND_GE, node, add());
        else
            return node;
    }
}

// add = mul ("+" mul | "-" mul)*
Node* add() {
    Node* node = mul();

    for (;;) {
        if (consume("+"))
            node = new_node_binary_op(ND_ADD, node, mul());
        else if (consume("-"))
            node = new_node_binary_op(ND_SUB, node, mul());
        else
            return node;
    }
}

// mul = unary ("*" unary | "/" unary)*
Node* mul() {
    Node* node = unary();

    for (;;) {
        if (consume("*"))
            node = new_node_binary_op(ND_MUL, node, unary());
        else if (consume("/"))
            node = new_node_binary_op(ND_DIV, node, unary());
        else
            return node;
    }
}

// unary = ("+" | "-")? primary
Node* unary() {
    if (consume("+"))
        return primary();
    if (consume("-"))
        return new_node_binary_op(ND_SUB, new_node_num(0), primary());
    return primary();
}

// primary = "(" expr ")" | ident func-args? | num
Node* primary() {
    if (consume("(")) {
        Node* node = expr();
        expect(")");
        return node;
    }

    Token* tok = consume_ident();
    if (tok) {
        if (consume("(")) {
            Node* node = new_node(ND_FUN_CALL);
            node->func_name = strndup(tok->str, tok->len);
            node->args = func_args();
            return node;
        }

        LocalVar* var = find_local_var(tok);
        if (!var) {
            var = push_local_var(tok);
        }
        return new_local_var(var);
    }

    return new_node_num(expect_number());
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
