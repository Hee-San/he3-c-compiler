#include "he3cc.h"

// ファイルの内容を読み込んで返す
char *read_file(char *path) {
  // Open and read the file.
  FILE *fp = fopen(path, "r");
  if (!fp)
    error("ファイルを開けません %s: %s", path, strerror(errno));

  int filemax = 10 * 1024 * 1024; // 10MB
  char *buf = malloc(filemax);
  int size = fread(buf, 1, filemax - 2, fp);
  if (!feof(fp))
    error("%s: ファイルが大きすぎます", path);

  // 文字列が "\n\0" で終わることを保証
  if (size == 0 || buf[size - 1] != '\n')
    buf[size++] = '\n';
  buf[size] = '\0';
  return buf;
}

int align_to(int n, int align) { return (n + align - 1) & ~(align - 1); }

int main(int argc, char **argv) {
  if (argc != 2) {
    error("引数の個数が正しくありません");
    return 1;
  }

  // ファイルから読み込む
  filename = argv[1];
  user_input = read_file(argv[1]);

  // トークナイズする
  token = tokenize();

  // パースする
  Program *prog = program();

  // 型を付ける
  add_type(prog);

  for (Function *fn = prog->fns; fn; fn = fn->next) {
    // ローカル変数のオフセットを決定する
    int offset = 0;
    for (VarList *var_list = fn->local_vars; var_list;
         var_list = var_list->next) {
      Var *var = var_list->var;
      offset += size_of(var->ty);
      var->offset = offset;
    }
    fn->local_var_stack_size = align_to(offset, 16);
  }
  // コード生成する
  codegen(prog);

  return 0;
}
