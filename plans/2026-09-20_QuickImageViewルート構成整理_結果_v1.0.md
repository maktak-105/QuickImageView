# QuickImageViewルート構成整理 実施結果 v1.0

## 実施内容
- Codexスキルを`.agents/skills/`へ移し、案内READMEのパスを更新。
- ルートの`plan.md`を`docs/loop/quality-plan.md`へ移し、ループREADMEからリンク。
- Qt導入ログを`build/intermediate/logs/aqtinstall.log`へ移動。
- 実ファイルがなかった`src/ui/`の空プレースホルダーを削除。Win32コントロール中心の現行実装に合わせ、構成資料を更新。
- MSI専用のWiX定義、ライセンス、ビルド処理を含む`installer/`は独立工程としてルートに保持。

## 確認
- ルートから`skills/`、`plan.md`、`aqtinstall.log`、空の`src/ui/`がなくなったことを確認。
- `docs/project-structure.md`と`docs/loop/README.md`の配置記述・リンクを更新。
- `git diff --check`を実施。アプリの再ビルドとUI検査は今回未実施。
