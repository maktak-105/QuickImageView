# QuickImageView 開発計画

## 目的

Windows上で画像を安全に表示し、原本を変更せずに別ファイルへ変換できる単体実行可能なMVPを作る。

## ループ

1. `docs/loop/goal.json` のFAIL項目を1つ選ぶ。
2. 実装を変更する。
3. `powershell -ExecutionPolicy Bypass -File .\tests\verify_goal.ps1` を実行する。
4. FAILが残る限り繰り返す。

テスト終了コードが0の場合だけ完了とする。過去のループ履歴、担当者間メッセージ、証拠転記、レビュー用ログは保持しない。

## 対象外

画面の見た目、Explorerの人間操作、未実装形式の実機変換は、再現可能な自動テストを用意するまで完了条件に含めない。
