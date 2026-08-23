# QuickImageView 配布手順

[English version distribution.md](distribution.md)

この文書は現在のソースツリーにおける配布手順です。72件の正式ベースラインや追加案件を、UI検査済みとは判定しません。

ZIPとMSIには、実行ファイル、日英README・履歴・ヘルプ、MIT License、日本語ライセンス文書、libwebpのCOPYING・PATENTSを同梱します。MSIではExplorer右クリック登録と対応拡張子ごとの関連付けを独立した任意機能として選択でき、既定ではすべて未選択です。PowerShellインストーラーは現在のユーザー（HKCU）へのインストールです。

## ビルド

```powershell
cmake -S . -B dist/binary -G "MinGW Makefiles"
cmake --build dist/binary --parallel 2
ctest --test-dir dist/binary --output-on-failure
```

## インストール

通常のインストールではアプリ本体だけをユーザー領域へ配置する。

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install.ps1
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
