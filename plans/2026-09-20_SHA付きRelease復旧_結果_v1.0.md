# SHA付きRelease復旧 結果

## 変更
- リポジトリをPublicに変更。
- CI/ReleaseのCMake generatorをrunner既定に変更。
- Release workflowにタグ指定のworkflow_dispatchとタグref checkoutを追加。

## 検証
- CIと`v3.1.3`手動Release workflowの結果を記録する。
- SHA添付とZIPハッシュ一致を実行後に記録する。
