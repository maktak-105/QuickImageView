# QuickImageView テスト手順

## 完了判定（アプリを起動する）

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tests\verify_goal.ps1
```

終了コード `0` だけを完了とする。出力に `FAIL` が1件でもあれば未完了。

要求管理ゲートを直接確認する場合:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tests\manage_loop.ps1
```

## ダッシュボード更新（アプリを起動しない）

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tests\generate_dashboard.ps1
```

生成物は `docs/loop/dashboard.html`。これは仕様・チェックリストの現在状態だけを表示する静的HTMLで、履歴ログではない。ダッシュボード生成では `verify_goal.ps1`、`manage_loop.ps1`、QIV本体を実行しない。

## 検査レイヤー

| レイヤー | 検査内容 | 主な検査 |
|---|---|---|
| 契約 | ゴールと維持契約のJSON、ID重複 | `goal_definition`, `regression_invariants` |
| 静的回帰 | 既存機能の実装プローブが残っているか | `tests/verify_goal.ps1` |
| ビルド | CMakeクリーンビルド、CTest | `build_configure`, `build`, `ctest` |
| 実データ | リサイズ、切り抜き、回転反転、色変換、Undo/Redo、クリップボード、品質保存 | `--self-test-edit` |
| ビルド版UI | ダイアログ入力、左ドラッグ選択、メニュー内容 | `tests/verify_ui.ps1` |
| 配布 | 一時インストール、レジストリ、SHA-256一致 | `context_menu_install`, `installed_parity` |
| インストール版UI | 配布後の実行ファイルを直接操作 | `installed_ui_dynamic` |

## 既存機能を変更する場合

ファイル情報・EXIF表示など、`docs/loop/invariants.json` にある機能は削除しない。変更が必要な場合は、先に仕様書と維持契約を更新し、ユーザーの明示承認を得る。検査を通すためだけに維持契約のプローブを削除してはならない。

## 個別UI検査

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tests\verify_ui.ps1
```

この検査はQIVをテスト画像付きで起動し、次を実際に操作する。

- Undo/Redoがメニューに存在しないこと
- 「リサイズを指定...」が開くこと
- ピクセル指定とパーセント指定を入力できること
- 左ドラッグ後に切り抜きコマンドを実行できること

## 失敗時の扱い

失敗した検査を削除・無効化してはいけない。原因を修正し、同じ完了コマンドを再実行する。結果ログや過去ログは保存しないが、現在の契約と検査プログラムは必ずリポジトリに残す。
