# QuickImageView 開発計画

## 完了条件

README.mdに記載した実装対象機能を、固定操作台帳の全件について、ビルド版とインストール版の実UIでPASSする。UI不合格をCLI・静的・関数・変数検査で覆さない。

## ループ

1. Plan: FAIL/ERRORのうち次に扱う1件を選び、成功条件を定義する。
2. Act: その1件のUI操作・結果確認を1つだけ実装する。
3. Observe: PowerShell 7.6.5で全ループを実行し、実測結果を確認する。
4. Reflect: 成功なら次の1件へ、失敗なら原因を特定して次の変更を変える。

実行入口:

```powershell
python .\python\tests\run_loop.py
```

## 検査順

ビルド → インストール → インストール済みアプリの全UI検査 → CLI・静的・関数・変数補助検査。

README.mdの機能を変更した場合だけ、`python/tests/rebuild_catalog.py`で`python/tests/operations.json`を再構築する。
