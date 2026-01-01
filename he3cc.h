#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Type Type;

//
// tokenize.c
//

// トークンの種類
typedef enum {
  TK_RESERVED, // 記号
  TK_IDENT,    // 識別子
  TK_NUM,      // 整数トークン
  TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

// トークン型
typedef struct Token Token;
struct Token {
  TokenKind kind; // トークンの型
  Token *next;    // 次の入力トークン
  int val;        // kindがTK_NUMの場合、その数値
  char *str;      // トークン文字列
  int len;        // トークンの文字数
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
Token *peek(char *s);
Token *consume(char *op);
void expect(char *op);
int expect_number();
char *expect_ident();
bool at_eof();
char *strndup(char *p, int len);
Token *consume_ident();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize();

extern char *user_input;
extern Token *token;

//
// parse.c
//

// 抽象構文木のノードの種類
typedef enum {
  ND_ADD,       // +
  ND_SUB,       // -
  ND_MUL,       // *
  ND_DIV,       // /
  ND_EQ,        // ==
  ND_NE,        // !=
  ND_LT,        // <
  ND_LE,        // <=
  ND_GT,        // >
  ND_GE,        // >=
  ND_ASSIGN,    // =
  ND_ADDR,      // & アドレス演算子
  ND_DEREF,     // * 間接参照演算子
  ND_RETURN,    // "return"
  ND_IF,        // "if"
  ND_WHILE,     // "while"
  ND_FOR,       // "for"
  ND_SIZEOF,    // "sizeof"
  ND_BLOCK,     // ブロック { ... }
  ND_FUN_CALL,  // 関数呼び出し
  ND_EXPR_STMT, // 式文
  ND_LOCAL_VAR, // ローカル変数
  ND_NUM,       // 整数
  ND_NULL,      // 空文
} NodeKind;

// 変数を表す型
typedef struct Var Var;
struct Var {
  char *name; // 変数名
  Type *ty;   // 型
  int offset; // RBP(ベースポインタ)からのオフセット
};

typedef struct VarList VarList;
struct VarList {
  VarList *next;
  Var *var;
};

// 抽象構文木のノードの型
typedef struct Node Node;
struct Node {
  NodeKind kind; // ノードの型
  Node *next;    // 次のノード
  Type *ty; // ノードの型情報 (現状はintまたはintへのポインタのみ)
  Token *tok; // ノードに対応するトークン

  Node *lhs; // 左辺 (left-hand side)
  Node *rhs; // 右辺 (right-hand side)
  Var *var;  // kindがND_LOCAL_VARの場合に使う変数名
  int val;   // kindがND_NUMの場合のみ使う数値

  // kindがND_IF, ND_WHILE, ND_FORの場合に使う
  Node *cond; // 条件式
  Node *then; // then節
  Node *els;  // else節
  Node *init; // 初期化文 (for文用)
  Node *inc;  // 増分文 (for文用)

  // kindがND_FUN_CALLの場合に使う
  char *func_name; // 関数名
  Node *args;      // 引数リスト

  Node *body; // kindがND_BLOCKの場合に使う文のリスト
};

// 関数を表す型
typedef struct Function Function;
struct Function {
  Function *next;
  char *name;
  VarList *params;

  Node *node;
  VarList *local_vars;
  int local_var_stack_size;
};

// プログラム全体を表す型
typedef struct {
  Function *fns;
} Program;

Program *program();

//
// type.c
//

typedef enum {
  TY_INT,
  TY_PTR,
  TY_ARRAY,
} TypeKind;

struct Type {
  TypeKind kind;
  Type *base;     // ポインタ型の場合、指している型
  int array_size; // 配列型の場合、要素数
};

Type *int_type();
Type *pointer_to(Type *base);
Type *array_of(Type *base, int size);
int size_of(Type *ty);

void add_type(Program *prog);

//
// codegen.c
//

void codegen(Program *prog);
