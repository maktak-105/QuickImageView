# QuickImageView 検査引き継ぎ

## 正本

- 機能仕様: `README.md`
- 固定操作台帳: `tests/operations.json`
- ループ設計: `plans/test-rebuild-python-001.md`
- 実行入口: `tests/run_loop.py`
- 直近結果: `docs/loop/current.json`
- ダッシュボード: `docs/loop/report.html`

README.mdの実装対象6セクションの箇条書き63件と操作台帳は1:1で一致している。通常実行では台帳を再生成しない。README.mdの機能追加・変更・削除時だけ、明示的に `python tests/rebuild_catalog.py` を実行し、差分を確認する。

## 実行順

1. ビルドする。
2. ビルド成果物をインストールする。
3. インストール済みアプリで台帳の全機能を実UIで1回だけ検査する。
4. UI検査終了後にだけ、CLI・静的・関数・変数・ビルド補助検査を実行する。
4. UIのFAIL/ERROR/UNCHECKEDは補助検査で覆さない。
5. `current.json` と `report.html` は直近結果だけを上書きする。過去履歴は保存しない。

## コマンド

```powershell
python .\tests\run_loop.py
```

PowerShell 7.6.5を使用する。ユーザーの明示なしにcommit・push・アプリ実装変更を行わない。
