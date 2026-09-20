# QuickImageViewルート構成の残件整理 v1.0

## 目的
quick-app-templateにないルート項目を用途別に整理し、実態のない空UIフォルダと構成資料のずれをなくす。

## 対象
- `skills/`をCodexのプロジェクトスキル配置`.agents/skills/`へ移す。
- 開発ループの現行計画`plan.md`を`docs/loop/quality-plan.md`へ移す。
- ルートに残る生成ログ`aqtinstall.log`を`build/intermediate/logs/`へ移す。
- UI素材を含まない空の`src/ui/`を削除し、READMEと構成図をWin32/WIC実装に合わせる。
- MSI専用の`installer/`は独立した配布工程としてルートに保持する。
