# QuickImageView 配布手順

[English version distribution.md](distribution.md)

QuickImageViewは、署名なしZIPとしてGitHub Releasesで配布します。インストーラーはありません。各Releaseには`QuickImageView-binary.zip`と、CIが生成した`SHA256SUMS.txt`を添付します。

ZIPには、`QuickImageView.exe`、配置済みのQt DLL・プラグイン（`platforms`、`imageformats`、`qml`）、日英README・履歴、MIT License、日本語ライセンス文書、第三者通知、libwebpのCOPYING・PATENTSを同梱します。日英ヘルプは実行ファイルに埋め込まれています。

## ビルド

```powershell
.\scripts\build.bat
```

ツールチェーンは[environment_jp.md](environment_jp.md)を参照してください。

## パッケージ化

```powershell
.\scripts\package.ps1 -OutputDirectory .\build\intermediate\package -ArchivePath .\build\QuickImageView-binary.zip
```

`package.ps1`は`dist/`から実行ファイルとQtランタイムを、`docs/distribution/`から配布文書を集めます。先に`build.bat`を実行し、`dist/`へQtランタイムを配置しておく必要があります。

## リリース

`v*`タグをpushすると`.github/workflows/release.yml`が動き、Qt 6.10.3（MSVC 2022）でビルド、CTest、Qtランタイム配置、ZIPと`SHA256SUMS.txt`の作成、Releaseへの公開を行います。既存タグを指定した手動実行もできます。

## インストール

PowerShellインストーラーは、実行ファイルとQtランタイムを現在のユーザーのLocalAppDataへ配置し、画像の右クリックメニューへ現在のユーザー（HKCU）として登録します。管理者権限は要求しません。

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install.ps1
```

右クリックメニューへ登録しない場合は次のとおりです。

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install.ps1 -NoRegisterContextMenu
```

PowerToysのImage Resizer／PowerRename／File LocksmithがWindows 11のモダン登録と従来登録を二重に表示する場合は、次を1回実行すると、モダン側だけを現在のユーザーで無効化し、従来項目を1件に整理できます。PowerToysやExplorerの再起動後も設定は保持されます。

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\cleanup-context-menu.ps1
```

## アンインストール

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\uninstall.ps1
```

アプリのユーザー領域とQuickImageView専用の右クリック登録だけを削除します。ユーザーが作成した画像ファイルは削除しません。
