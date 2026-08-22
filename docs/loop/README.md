# QuickImageView 開発ループ

ここには現在のゴール、維持必須機能、現在の状態だけを置く。過去ログ、結果ログ、レーン、メッセージ、証拠ファイルは置かない。

## 正本

- `goal.json`: 完了条件。
- `invariants.json`: 既存機能を削除していないことを検査する維持契約。
- `state.json`: 現在の状態。
- `tests/manage_loop.ps1`: 要求管理とリリース判定の唯一のゲート。
- `tests/verify_goal.ps1`: ビルド・静的検査・動的検査を実行する下位検査。
- `tests/verify_ui.ps1`: 実行中QIVへの動的UI検査。
- `tests/generate_dashboard.ps1`: アプリやテストを起動せず、現在の仕様・チェックリストだけから静的HTMLを生成する。

## 実行

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tests\manage_loop.ps1
```

終了コード `0` が完了、`1` が未完了。テストが実行できない場合も `1` とする。アプリ起動を伴う検査はこのコマンドを明示的に実行した時だけ行う。

ダッシュボードだけを更新する場合:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tests\generate_dashboard.ps1
```

このコマンドはアプリもテストも起動しない。検査結果を履歴として保存せず、未検査項目はダッシュボード上で `未検査` と表示する。

## 完了ゲート

```text
goal.json / invariants.json / checklist.jsonを読む
  ↓
ソース回帰検査（既存機能の削除を検出）
  ↓
クリーンビルド + CTest
  ↓
実データセルフテスト（編集・変換・Undo/Redo・クリップボード）
  ↓
ビルド版UI操作（ダイアログ・選択・メニュー・ショートカット）
  ↓
一時インストール + SHA-256一致
  ↓
インストール版UI操作
  ↓
チェックリストの必須項目と全検査がPASSの場合だけ完了
```

1つでもFAILなら未完了。検査を省略してPASSにしない。人間の記憶、過去結果、スクリーンショット、レビュー文書は判定に使わない。

機能を削除・変更する場合は、先に `goal.json` と `invariants.json` の契約を更新し、ユーザーの明示承認を得る。曖昧な「編集不要」は表示・読み取り・コピーまで削除する根拠にしない。
