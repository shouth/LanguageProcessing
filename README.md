# 言語処理プログラミング実装

[2021年度の言語処理プログラミング](https://www.syllabus.kit.ac.jp/?c=detail&pk=1157&schedule_code=12221202&sk=99&sn=%E8%A8%80%E8%AA%9E%E5%87%A6%E7%90%86%E3%83%97%E3%83%AD%E3%82%B0%E3%83%A9%E3%83%9F%E3%83%B3%E3%82%B0)の課題として実装したコンパイラです．
Pascalベースの言語「MPPL」をCASL2(にいくつか命令を加えたもの)に変換します．

本実装では

1. 入力として与えられたプログラムを `source` として抽象化
3. `lexer` で `cursol` を用いて `source` からトークン `token` を生成
4. `parser` で `token` から構文規則を確認しつつ抽象構文木 `ast` を生成
5. `analyzer` で `ast` から制約を確認しつつ中間表現 `ir` を生成
6. `casl2_codegen` で `ir` からCASL2のコードを生成

します．
