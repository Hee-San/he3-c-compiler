#include "he3cc.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        error("引数の個数が正しくありません");
        return 1;
    }

    // トークナイズする
    user_input = argv[1];
    token = tokenize();

    // パースする
    Node* node = expr();

    // コード生成する
    codegen(node);

    return 0;
}
