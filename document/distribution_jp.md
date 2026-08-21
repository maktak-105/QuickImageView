# QuickImageView 配布手順

## ビルド

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

## インストール

通常のインストールではアプリ本体だけをユーザー領域へ配置する。

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install.ps1
```

画像の右クリックメニューへ登録する場合は、ユーザーが明示的にスイッチを指定する。

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install.ps1 -RegisterContextMenu
```

管理者権限は要求しない。登録先は現在のユーザー（HKCU）に限定する。

## アンインストール

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\uninstall.ps1
```

アプリのユーザー領域とQuickImageView専用の右クリック登録だけを削除する。ユーザーが作成した画像ファイルは削除しない。
