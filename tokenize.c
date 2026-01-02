#include "he3cc.h"

// 現在着目しているトークン
Token *token;

// 入力ファイル名
char *filename;

// 入力プログラム
char *user_input;

// エラーを報告するための関数
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// エラーメッセージを以下の形式で出力:
//
// foo.c:10: x = y + 1;
//           ^ エラーメッセージ
void verror_at(char *loc, char *fmt, va_list ap) {
  // loc を含む行を探す
  char *line = loc;
  while (user_input < line && line[-1] != '\n')
    line--;

  char *end = loc;
  while (*end != '\n')
    end++;

  // 行番号を取得
  int line_num = 1;
  for (char *p = user_input; p < line; p++)
    if (*p == '\n')
      line_num++;

  // 行を出力
  int indent = fprintf(stderr, "%s:%d: ", filename, line_num);
  fprintf(stderr, "%.*s\n", (int)(end - line), line);

  // エラー位置を示す
  int pos = loc - line + indent;
  fprintf(stderr, "%*s", pos, "");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// エラー箇所を報告する
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(loc, fmt, ap);
}

// エラー箇所を報告する
void error_tok(Token *tok, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (tok) {
    verror_at(tok->str, fmt, ap);
  }

  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// 次のトークンが期待している記号のときには、
// 真を返す。それ以外の場合には偽を返す。
Token *peek(char *s) {
  if (token->kind != TK_RESERVED || strlen(s) != token->len ||
      memcmp(token->str, s, token->len))
    return NULL;
  return token;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
Token *consume(char *s) {
  if (!peek(s))
    return NULL;

  Token *t = token;
  token = token->next;
  return t;
}

// 次のトークンが識別子の場合、トークンを1つ読み進めてそのトークンを返す。
// それ以外の場合には偽を返す。
Token *consume_ident() {
  if (token->kind != TK_IDENT)
    return NULL;
  Token *tok = token;
  token = token->next;
  return tok;
}

// 次のトークンが期待している記号のときには、トークンを1つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *s) {
  if (!peek(s))
    error_tok(token, "'%s'が必要です", s);
  token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
  if (token->kind != TK_NUM)
    error_tok(token, "数が必要です");
  int val = token->val;
  token = token->next;
  return val;
}

// 次のトークンが識別子の場合、トークンを1つ読み進めてその文字列を返す。
// それ以外の場合にはエラーを報告する。
char *expect_ident() {
  if (token->kind != TK_IDENT)
    error_tok(token, "識別子が必要です");
  char *s = duplicate_string_n(token->str, token->len);
  token = token->next;
  return s;
}

bool at_eof() { return token->kind == TK_EOF; }

// 文字列pの長さlenの部分文字列をコピーして新しい文字列を作成して返す
char *duplicate_string_n(char *p, int len) {
  char *buf = malloc(len + 1);
  strncpy(buf, p, len);
  buf[len] = '\0';
  return buf;
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

bool startswith(char *p, char *q) { return strncmp(p, q, strlen(q)) == 0; }

bool is_alpha(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

bool is_alnum(char c) { return is_alpha(c) || ('0' <= c && c <= '9'); }

// 予約語をマッチングして新しいトークンを返す。マッチしなければNULLを返す
Token *try_keyword(Token *cur, char **p) {
  static char *keywords[] = {
      "return", "if", "else", "while", "for", "int", "char", "sizeof",
  };
  for (int i = 0; i < sizeof(keywords) / sizeof(*keywords); i++) {
    int len = strlen(keywords[i]);
    if (startswith(*p, keywords[i]) && !is_alnum((*p)[len])) {
      Token *tok = new_token(TK_RESERVED, cur, *p, len);
      *p += len;
      return tok;
    }
  }
  return NULL;
}

// 2文字の演算子をマッチングして新しいトークンを返す。マッチしなければNULLを返す
Token *try_multi_char_op(Token *cur, char **p) {
  static char *multi_char_ops[] = {
      "==",
      "!=",
      "<=",
      ">=",
  };
  for (int i = 0; i < sizeof(multi_char_ops) / sizeof(*multi_char_ops); i++) {
    int len = strlen(multi_char_ops[i]);
    if (startswith(*p, multi_char_ops[i])) {
      Token *tok = new_token(TK_RESERVED, cur, *p, len);
      *p += len;
      return tok;
    }
  }
  return NULL;
}

// エスケープシーケンスを解釈して対応する文字を返す
char get_escape_char(char c) {
  switch (c) {
  case 'a':
    return '\a';
  case 'b':
    return '\b';
  case 't':
    return '\t';
  case 'n':
    return '\n';
  case 'v':
    return '\v';
  case 'f':
    return '\f';
  case 'r':
    return '\r';
  case 'e':
    return 27;
  case '0':
    return 0;
  default:
    return c;
  }
}

Token *read_string_literal(Token *cur, char *start) {
  char *p = start + 1; // 開始の " をスキップ
  char buf[1024];
  int len = 0;

  for (;;) {
    if (len == sizeof(buf))
      error_at(start, "文字列リテラルは1024文字以下");
    if (*p == '\0')
      error_at(start, "文字列の終端がありません");
    if (*p == '"')
      break;

    if (*p == '\\') {
      p++;
      buf[len++] = get_escape_char(*p++);
    } else {
      buf[len++] = *p++;
    }
  }

  Token *tok = new_token(TK_STR, cur, start, p - start + 1);
  tok->contents = malloc(len + 1);
  memcpy(tok->contents, buf, len);
  tok->contents[len] = '\0';
  tok->contents_len = len + 1;
  return tok;
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize() {
  char *p = user_input;
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    // 空白文字をスキップ
    if (isspace(*p)) {
      p++;
      continue;
    }

    //　コメント文をスキップ
    if (startswith(p, "//")) {
      p += 2;
      while (*p != '\n')
        p++;
      continue;
    }

    // ブロックコメントをスキップ
    if (startswith(p, "/*")) {
      char *q = strstr(p + 2, "*/");
      if (!q)
        error_at(p, "コメント文の終端がありません");
      p = q + 2;
      continue;
    }

    // 予約語
    Token *tok = try_keyword(cur, &p);
    if (tok) {
      cur = tok;
      continue;
    }

    // 2文字の記号
    tok = try_multi_char_op(cur, &p);
    if (tok) {
      cur = tok;
      continue;
    }

    // 1文字の記号
    if (strchr("+-*/()<>;={},&[]", *p)) {
      cur = new_token(TK_RESERVED, cur, p, 1);
      p++;
      continue;
    }

    // 識別子
    if (is_alpha(*p)) {
      char *start = p;
      p++;
      while (is_alnum(*p)) {
        p++;
      }
      cur = new_token(TK_IDENT, cur, start, p - start);
      continue;
    }

    // 文字列リテラル
    if (*p == '"') {
      cur = read_string_literal(cur, p);
      p += cur->len;
      continue;
    }

    // 数字
    if (isdigit(*p)) {
      char *num_start = p;
      long val = strtol(p, &p, 10);
      int len = (int)(p - num_start);
      cur = new_token(TK_NUM, cur, num_start, len);
      cur->val = val;
      continue;
    }

    error_at(p, "トークナイズできません");
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
}
