#include "he3cc.h"

Node* new_node(NodeKind kind) {
    Node* node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

Node* new_node_binary_op(NodeKind kind, Node* lhs, Node* rhs) {
    Node* node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node* new_node_num(int val) {
    Node* node = new_node(ND_NUM);
    node->val = val;
    return node;
}

// program    = stmt*
// stmt       = expr ";"
// expr       = equality
// equality   = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add        = mul ("+" mul | "-" mul)*
// mul        = unary ("*" unary | "/" unary)*
// unary      = ("+" | "-")? primary
// primary    = num | "(" expr ")"

Node* stmt();
Node* expr();
Node* equality();
Node* relational();
Node* add();
Node* mul();
Node* unary();
Node* primary();

Node* code[100];

// program = stmt*
void program() {
    int i = 0;
    while (!at_eof()) {
        code[i++] = stmt();
    }
    code[i] = NULL;
}

// stmt = expr ";"
Node* stmt() {
    Node* node = expr();
    expect(";");
    return node;
}

// expr = equality
Node* expr() {
    return equality();
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

// primary = "(" expr ")" | num
Node* primary() {
    if (consume("(")) {
        Node* node = expr();
        expect(")");
        return node;
    }

    return new_node_num(expect_number());
}
