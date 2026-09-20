# MSIをReleaseへ添付してSHAへ追加 結果

## 実施内容
- Release workflowにWiX Toolset v3の導入、MSIビルド、MSIのSHA-256生成、Release添付を追加。
- READMEと日英変更履歴を更新。

## 検証
- GitHub Actions Release run `35494787950` 成功。WiX MSI生成とRelease添付を確認。
- Release URL: https://github.com/maktak-105/QuickImageView/releases/tag/v3.1.3
- `SHA256SUMS.txt`内のZIP、MSI、EXEをダウンロードした実配布物と照合し、3/3一致。
