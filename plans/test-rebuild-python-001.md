# Python UI検査環境 再構築計画

## 目的

README.mdの実装対象機能を、実際のインストール済みQuickImageViewのUIで検査する。検査行を走査しただけのPASS、関数・変数・ソース確認だけのPASS、未実装操作のPASSを禁止する。

## 実行フロー

1. README.mdと固定台帳の一致を検証する。
2. CMake構成・ビルドを行う。
3. ビルド成果物を一時インストールする。
4. インストール済みアプリを通常起動する。
5. UIAでメニュー・ダイアログを特定し、キャンバスだけ実マウス・実キー入力で操作する。
6. 操作後に画面、表示テキスト、寸法、クリップボード、出力ファイルを確認する。
7. UI検査が終わった後だけ、CLI・静的・関数・変数・CTestを補助検査する。
8. 現在結果とHTMLレポートだけを上書きする。

## 判定

- `PASS`: UI操作と観測可能な結果確認が両方成功。
- `FAIL`: UI操作を実行し、期待結果と異なった。
- `ERROR`: UI起動または検査実行でエラー。
- `UNCHECKED`: その機能のUI操作・結果確認がまだ実装されていない。PASSやFAILに変換しない。
- 全体PASSは全機能がPASSの場合だけ。

## 正本

- 機能仕様: `README.md`
- 固定操作台帳: `python/tests/operations.json`
- Python実行入口: `python/tests/run_loop.py`
- UI検査: `python/tests/ui_test.py`
- レポート: `docs/loop/current.json`, `docs/loop/report.html`

README.mdの機能変更時だけ、明示的な台帳再構築を行う。通常実行で機能行を生成しない。
