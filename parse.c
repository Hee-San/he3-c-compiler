#include "he3cc.h"

// local_vars: 関数内の全ローカル変数
//             スタックサイズ計算・オフセット割り当てに使う
//             ブロックを抜けても変数は残る（メモリは確保したまま）
VarList *local_vars;

// global_vars: 全グローバル変数
//              .data セクションに出力される
VarList *global_vars;

// scope_vars: 現在のスコープで「見える」変数
//             find_var() での名前解決に使う
//             ブロックを抜けると復元される（シャドウイング対応）
VarList *scope_vars;

// 既存の変数を名前で検索する関数
Var *find_var(Token *tok) {
  for (VarList *var_list = scope_vars; var_list; var_list = var_list->next) {
    Var *var = var_list->var;
    if (tok->len == strlen(var->name) &&
        !memcmp(tok->str, var->name, tok->len)) {
      return var;
    }
  }
  return NULL;
}

// 新しいノードを生成する関数
Node *new_node(NodeKind kind, Token *tok) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->tok = tok;
  return node;
}

// 二項演算子ノードを生成する関数
Node *new_node_binary_op(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
  Node *node = new_node(kind, tok);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

// 単項演算子ノードを生成する関数
Node *new_node_unary_op(NodeKind kind, Node *operand, Token *tok) {
  Node *node = new_node(kind, tok);
  node->lhs = operand;
  return node;
}

// 数値ノードを生成する関数
Node *new_node_num(int val, Token *tok) {
  Node *node = new_node(ND_NUM, tok);
  node->val = val;
  return node;
}

// 変数ノードを生成する関数
Node *new_var(Var *var, Token *tok) {
  Node *node = new_node(ND_VAR, tok);
  node->var = var;
  return node;
}

// 新しい変数を変数リストに追加する関数
Var *push_var(char *name, Type *ty, bool is_local) {
  Var *var = calloc(1, sizeof(Var));
  var->name = name;
  var->ty = ty;
  var->is_local = is_local;

  VarList *var_list = calloc(1, sizeof(VarList));
  var_list->var = var;
  if (is_local) {
    var_list->next = local_vars;
    local_vars = var_list;
  } else {
    var_list->next = global_vars;
    global_vars = var_list;
  }

  VarList *sc = calloc(1, sizeof(VarList));
  sc->var = var;
  sc->next = scope_vars;
  scope_vars = sc;

  return var;
}

// 新しい匿名ラベルを生成する関数
char *new_label() {
  static int cnt = 0;
  char buf[20];
  sprintf(buf, "data.%d", cnt++);
  return duplicate_string_n(buf, 20);
}

// トップレベル
Program *program();
void global_var();
Function *function();

// 型
Type *basetype();
Type *type_suffix(Type *base);

// 関数パラメータ
VarList *func_params();
VarList *func_param();

// 文
Node *stmt();
Node *declaration();

// 式
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *postfix();
Node *primary();

// GNU拡張の式文
Node *stmt_expr();

// 関数呼び出し引数
Node *func_args();

// 先読み: 関数定義かどうか
bool peek_is_function() {
  Token *saved = token;
  basetype();
  bool result = consume_ident() && consume("(");
  token = saved;
  return result;
}

// program = function*
Program *program() {
  Function head;
  head.next = NULL;
  Function *cur = &head;
  global_vars = NULL;

  while (!at_eof()) {
    if (peek_is_function()) {
      cur->next = function();
      cur = cur->next;
    } else {
      global_var();
    }
  }

  Program *prog = calloc(1, sizeof(Program));
  prog->global_vars = global_vars;
  prog->fns = head.next;
  return prog;
}

// global-var = basetype ident ("[" num "]")* ("=" expr)? ";"
void global_var() {

  Type *ty = basetype();
  char *name = expect_ident();
  ty = type_suffix(ty);
  expect(";");
  push_var(name, ty, false);
}

// function = basetype ident "(" func-params? ")" "{" stmt* "}"
Function *function() {
  local_vars = NULL;

  Function *fn = calloc(1, sizeof(Function));
  basetype();
  fn->name = expect_ident();
  expect("(");

  fn->params = func_params();
  expect("{");

  Node head;
  head.next = NULL;
  Node *cur = &head;

  while (!consume("}")) {
    cur->next = stmt();
    cur = cur->next;
  }

  fn->node = head.next;
  fn->local_vars = local_vars;
  return fn;
}

// basetype = ("int" | "char") "*"*
Type *basetype() {
  Type *ty;
  if (consume("char")) {
    ty = char_type();
  } else {
    expect("int");
    ty = int_type();
  }

  while (consume("*")) {
    ty = pointer_to(ty);
  }
  return ty;
}

// type-suffix = ("[" num "]")*
Type *type_suffix(Type *base) {
  if (!consume("[")) {
    return base;
  }
  int sz = expect_number();
  expect("]");
  base = type_suffix(base);
  return array_of(base, sz);
}

// func-params = ident ("," ident)*
VarList *func_params() {
  if (consume(")")) {
    return NULL;
  }

  VarList *head = func_param();
  VarList *cur = head;

  while (!consume(")")) {
    expect(",");
    cur->next = func_param();
    cur = cur->next;
  }

  return head;
}

// func-param = basetype ident
VarList *func_param() {
  Type *ty = basetype();
  char *name = expect_ident();
  ty = type_suffix(ty);

  VarList *vl = calloc(1, sizeof(VarList));
  vl->var = push_var(name, ty, true);
  return vl;
}

bool is_type_name() { return peek("int") || peek("char"); }

// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "{" stmt* "}"
//      | declaration
//      | expr ";"
Node *stmt() {
  Token *tok;
  if ((tok = consume("return"))) {
    Node *node = new_node_unary_op(ND_RETURN, expr(), tok);
    expect(";");
    return node;
  }

  if ((tok = consume("if"))) {
    Node *node = new_node(ND_IF, tok);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    if (consume("else")) {
      node->els = stmt();
    }
    return node;
  }

  if ((tok = consume("while"))) {
    Node *node = new_node(ND_WHILE, tok);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    return node;
  }

  if ((tok = consume("for"))) {
    Node *node = new_node(ND_FOR, tok);
    expect("(");
    if (!consume(";")) {
      node->init = new_node_unary_op(ND_EXPR_STMT, expr(), tok); 
      expect(";");
    }
    if (!consume(";")) {
      node->cond = expr();
      expect(";");
    }
    if (!consume(")")) {
      node->inc = new_node_unary_op(ND_EXPR_STMT, expr(), tok);
      expect(")");
    }
    node->then = stmt();
    return node;
  }

  if ((tok = consume("{"))) {
    Node *node = new_node(ND_BLOCK, tok);
    Node head;
    head.next = NULL;
    Node *cur = &head;

    // 新しいスコープを作成
    VarList *sc = scope_vars;

    while (!consume("}")) {
      cur->next = stmt();
      cur = cur->next;
    }

    // スコープを復元
    scope_vars = sc;

    node->body = head.next;
    return node;
  }

  if (is_type_name()) {
    return declaration();
  }

  tok = token;
  Node *node = new_node_unary_op(ND_EXPR_STMT, expr(), tok);
  expect(";");
  return node;
}

// declaration = basetype ident ("[" num "]")* ("=" expr)? ";"
Node *declaration() {
  Token *tok = token;
  Type *ty = basetype();
  char *name = expect_ident();
  ty = type_suffix(ty);
  Var *var = push_var(name, ty, true);

  if (consume(";")) {
    return new_node(ND_NULL, tok);
  }

  expect("=");
  Node *lhs = new_var(var, tok);
  Node *rhs = expr();
  expect(";");
  Node *node = new_node_binary_op(ND_ASSIGN, lhs, rhs, tok);
  return new_node_unary_op(ND_EXPR_STMT, node, tok);
}

// expr = assign
Node *expr() { return assign(); }

// assign = equality ("=" assign)?
Node *assign() {
  Node *node = equality();
  Token *tok;
  if ((tok = consume("=")))
    node = new_node_binary_op(ND_ASSIGN, node, assign(), tok);
  return node;
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality() {
  Node *node = relational();
  Token *tok;

  for (;;) {
    if ((tok = consume("==")))
      node = new_node_binary_op(ND_EQ, node, relational(), tok);
    else if ((tok = consume("!=")))
      node = new_node_binary_op(ND_NE, node, relational(), tok);
    else
      return node;
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
  Node *node = add();
  Token *tok;

  for (;;) {
    if ((tok = consume("<")))
      node = new_node_binary_op(ND_LT, node, add(), tok);
    else if ((tok = consume("<=")))
      node = new_node_binary_op(ND_LE, node, add(), tok);
    else if ((tok = consume(">")))
      node = new_node_binary_op(ND_GT, node, add(), tok);
    else if ((tok = consume(">=")))
      node = new_node_binary_op(ND_GE, node, add(), tok);
    else
      return node;
  }
}

// add = mul ("+" mul | "-" mul)*
Node *add() {
  Node *node = mul();
  Token *tok;

  for (;;) {
    if ((tok = consume("+")))
      node = new_node_binary_op(ND_ADD, node, mul(), tok);
    else if ((tok = consume("-")))
      node = new_node_binary_op(ND_SUB, node, mul(), tok);
    else
      return node;
  }
}

// mul = unary ("*" unary | "/" unary)*
Node *mul() {
  Node *node = unary();
  Token *tok;

  for (;;) {
    if ((tok = consume("*")))
      node = new_node_binary_op(ND_MUL, node, unary(), tok);
    else if ((tok = consume("/")))
      node = new_node_binary_op(ND_DIV, node, unary(), tok);
    else
      return node;
  }
}

// unary = ("+" | "-" | "*" | "&")? unary | postfix
Node *unary() {
  Token *tok;
  if ((tok = consume("+")))
    return unary();
  if ((tok = consume("-")))
    return new_node_binary_op(ND_SUB, new_node_num(0, tok), unary(), tok);
  if ((tok = consume("*")))
    return new_node_unary_op(ND_DEREF, unary(), tok);
  if ((tok = consume("&")))
    return new_node_unary_op(ND_ADDR, unary(), tok);
  return postfix();
}

// postfix = primary ("[" expr "]")*
Node *postfix() {
  Node *node = primary();
  Token *tok;

  while ((tok = consume("["))) {
    // x[i] は *(x+i) の構文糖衣
    Node *exp = new_node_binary_op(ND_ADD, node, expr(), tok);
    expect("]");
    node = new_node_unary_op(ND_DEREF, exp, tok);
  }
  return node;
}

// 先読み: stmt-expr かどうか
bool peek_is_stmt_expr() {
  Token *saved = token;
  bool result = consume("(") && consume("{");
  token = saved;
  return result;
}

// primary = "(" "{" stmt-expr-tail
//         | "(" expr ")"
//         | "sizeof" unary
//         | ident func-args?
//         | str
//         | num
Node *primary() {
  Token *tok;

  if (peek_is_stmt_expr())
    return stmt_expr();

  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  if ((tok = consume_ident())) {
    if (consume("(")) {
      Node *node = new_node(ND_FUN_CALL, tok);
      node->func_name = duplicate_string_n(tok->str, tok->len);
      node->args = func_args();
      return node;
    }

    Var *var = find_var(tok);
    if (!var) {
      error_tok(tok, "未定義の変数です");
    }
    return new_var(var, tok);
  }

  if ((tok = consume("sizeof"))) {
    return new_node_unary_op(ND_SIZEOF, unary(), tok);
  }

  tok = token;
  if (tok->kind == TK_STR) {
    token = token->next;

    // char[N] 型を作る
    Type *ty = array_of(char_type(), tok->contents_len);
    // 匿名グローバル変数として登録
    Var *var = push_var(new_label(), ty, false);
    var->contents = tok->contents;
    var->contents_len = tok->contents_len;
    return new_var(var, tok);
  }

  if (tok->kind != TK_NUM)
    error_tok(tok, "式が必要です");

  return new_node_num(expect_number(), tok);
}

// stmt-expr = "(" "{" stmt+ "}" ")"
Node *stmt_expr() {
  Token *tok = token;
  expect("(");
  expect("{");

  VarList *sc = scope_vars;

  Node *node = new_node(ND_STMT_EXPR, tok);
  node->body = stmt();
  Node *cur = node->body;

  while (!consume("}")) {
    cur->next = stmt();
    cur = cur->next;
  }
  expect(")");

  scope_vars = sc;

  if (cur->kind != ND_EXPR_STMT)
    error_tok(cur->tok, "voidを返すstatement expressionはサポートしていません");
  *cur = *cur->lhs;
  return node;
}

// func-args = "(" (assign ("," assign)*)? ")"
Node *func_args() {
  if (consume(")")) {
    return NULL;
  }

  Node *head = assign();
  Node *cur = head;
  while (consume(",")) {
    cur->next = assign();
    cur = cur->next;
  }
  expect(")");
  return head;
}
