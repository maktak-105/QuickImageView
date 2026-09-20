# SHA付きRelease復旧

## 目的
- QuickImageViewリポジトリを公開する。
- Windows runnerでCMake構成が失敗しないよう固定したVisual Studio 2022 Generator指定を除去する。
- 手動実行で既存タグを指定できるようにし、既存`v3.1.3`タグをSHA付きReleaseとして公開する。

## 確認項目
- GitHub上で可視性がPublicになっている。
- CIとRelease workflowが成功する。
- ReleaseにZIPとSHA256SUMS.txtが添付され、SHAが一致する。
