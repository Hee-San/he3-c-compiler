#include "he3cc.h"

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

Node* new_local_var(char name) {
    Node* node = new_node(ND_LOCAL_VAR);
    node->name = name;
    return node;
}

// program    = stmt*
// stmt       = "return" expr ";" | expr ";"
// expr       = assign
// assign     = equality ("=" assign)?
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-")? primary
// primary    = "(" expr ")" | ident | num

Node* stmt();
Node* expr();
Node* assign();
Node* equality();
Node* relational();
Node* add();
Node* mul();
Node* unary();
Node* primary();

// program = stmt*
Program* program() {
    Node head;
    head.next = NULL;
    Node* cur = &head;

    while (!at_eof()) {
        cur->next = stmt();
        cur = cur->next;
    }

    Program* prog = calloc(1, sizeof(Program));
    prog->node = head.next;
    return prog;
}

// stmt = "return" expr ";" | expr ";"
Node* stmt() {
    if (consume("return")) {
        Node* node = new_node_unary_op(ND_RETURN, expr());
        expect(";");
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

// primary = "(" expr ")" | ident | num
Node* primary() {
    if (consume("(")) {
        Node* node = expr();
        expect(")");
        return node;
    }

    Token* tok = consume_ident();
    if (tok)
        return new_local_var(*tok->str);

    return new_node_num(expect_number());
}
