# MSIをReleaseへ添付してSHAへ追加 結果

## 実施内容
- Release workflowにWiX Toolset v3の導入、MSIビルド、MSIのSHA-256生成、Release添付を追加。
- READMEと日英変更履歴を更新。

## 検証
- GitHub Actions Release実行とMSIの添付を確認後に記録する。
- ReleaseのSHA一覧を配布ZIP、MSI、EXEの実ファイルと照合して結果を記録する。
