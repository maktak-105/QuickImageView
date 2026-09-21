# QuickImageView 配布手順

[English version distribution.md](distribution.md)

この文書は現在のQuickImageViewをビルド、インストール、アンインストールする手順です。

ZIPとMSIには、日英ヘルプを埋め込んだ実行ファイル、日英README・履歴、MIT License、日本語ライセンス文書、libwebpのCOPYING・PATENTSを同梱します。MSIではExplorer右クリック登録と対応拡張子ごとの関連付けを独立した任意機能として選択でき、既定ではすべて未選択です。PowerShellインストーラーは現在のユーザー（HKCU）へのインストールです。

## ビルド

```powershell
cmake -S . -B build/intermediate/native -G "MinGW Makefiles"
cmake --build build/intermediate/native --parallel 2
ctest --test-dir build/intermediate/native --output-on-failure
```

## インストール

通常のインストールではアプリ本体だけをユーザー領域へ配置する。

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install.ps1
```

Qt Quick版を右クリック登録の起動対象にする場合は、Qtランタイムも同じユーザー領域へ配置する。

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install.ps1 -Qt
```

PowerToysのImage Resizer／PowerRename／File LocksmithがWindows 11のモダン登録と従来登録を二重に表示する場合は、次を1回実行すると、モダン側だけを現在のユーザーで無効化し、従来項目を1件に整理できます。PowerToysやExplorerの再起動後も設定は保持されます。

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\cleanup-context-menu.ps1
```

画像の右クリックメニューへは既定で登録します。登録先は現在のユーザー（HKCU）に限定され、管理者権限は要求しません。
登録しない場合は `-NoRegisterContextMenu` を指定します。

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install.ps1 -NoRegisterContextMenu
```

## アンインストール

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\uninstall.ps1
```

アプリのユーザー領域とQuickImageView専用の右クリック登録だけを削除する。ユーザーが作成した画像ファイルは削除しない。

## Qt Quick版ZIP

`build.bat qt`の後、Qtランタイムを含むQt Quick版ZIPを作成できます。

```powershell
.\scripts\package.ps1 -Qt -OutputDirectory .\build\package-qt -ArchivePath .\build\QuickImageViewQt-v3.1.3-win64.zip
```

Qt版パッケージには`QuickImageViewQt.exe`、配布用Qt DLL・プラグイン・QMLモジュールが入り、旧Win32実行ファイルは入りません。署名なしZIPの配布方式を維持します。MSIは引き続き安定版Win32実行ファイルを対象とし、ユーザー単位インストーラーは`-Qt`指定時にQt版へ切り替えられます。
