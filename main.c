#include "he3cc.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    error("引数の個数が正しくありません");
    return 1;
  }

  // トークナイズする
  user_input = argv[1];
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
    fn->local_var_stack_size = offset;
  }
  // コード生成する
  codegen(prog);

  return 0;
}
