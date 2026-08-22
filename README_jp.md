# QuickImageView

[English README.md](README.md)

Windows向けの軽量画像ビューアーです。画像表示、ズーム、パン、画像編集、別ファイル変換、右クリック保存に対応しています。

リサイズはパーセントまたはピクセル指定に対応し、色変換、Undo/Redo、クリップボード、WebP/HEIC変換も利用できます。WebP/HEICはWindows側のWICコーデックが利用可能な場合に変換できます。

## ソースからビルド

```powershell
.\build.bat
```

詳細は [document/environment_jp.md](document/environment_jp.md) を参照してください。

## 配布

配布用ファイルは `dist/binary/`、配布文書は `dist/documents/` に配置します。正式リリースはGitHub ReleasesのZIPで配布します。

## ライセンス

MIT Licenseです。詳細は [LICENSE](LICENSE) を参照してください。
