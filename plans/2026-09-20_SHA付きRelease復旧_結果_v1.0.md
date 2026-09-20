# SHA付きRelease復旧 結果

## 変更
- リポジトリをPublicに変更。
- CI/ReleaseのCMake generatorをrunner既定に変更。
- Release workflowにタグ指定のworkflow_dispatchとタグref checkoutを追加。

## 検証
- CI成功、`v3.1.3`手動Release workflow成功。
- Release URL: https://github.com/maktak-105/QuickImageView/releases/tag/v3.1.3
- `QuickImageView-binary.zip`と同梱EXEを実ファイルで照合し、SHA-256が2/2一致。
