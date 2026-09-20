# MSIをReleaseへ添付してSHAへ追加

## 目的
- QuickImageViewのMSIインストーラーをGitHub Actionsでビルドし、ZIPと一緒にGitHub Releaseへ添付する。
- `SHA256SUMS.txt`にMSI、ZIP、EXEのハッシュを記録する。
- 英語・日本語READMEと変更履歴の配布説明を同期する。

## 確認項目
- WiX Toolset v3をActionsで用意し、既存`installer/build-msi.ps1`でMSIを生成できる。
- ReleaseにMSIが添付され、SHA一覧のMSIハッシュが実ファイルと一致する。
- ZIP・MSI・EXEの各ハッシュが実配布物と一致する。
