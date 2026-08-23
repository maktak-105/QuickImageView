# QuickImageView 検査ループ

検査ループの設計は [plans/test-rebuild-python-001.md](../../plans/test-rebuild-python-001.md) に定義する。

README.mdを機能仕様の正本とし、その1:1対応を固定した `python/tests/operations.json` を仕様台帳とする。通常実行では機能行を再生成しない。README.mdで機能を追加・変更・削除したときだけ、明示的な台帳再構築を行う。アプリ起動UI検査からはインストール・アンインストール時仕様を除外し、アプリ起動仕様はUIで検査する。除外対象は `docs/loop/current.json` の `excluded_from_app_ui` に記録する。

実行入口:

```powershell
python .\python\tests\run_loop.py
```

順序はビルド→インストール→インストール済みアプリの対象UI検査→補助検査。`UNCHECKED`をPASSに変換しない。インストール・アンインストール時仕様は別のインストール検査へ分離する。
