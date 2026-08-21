# QuickImageView

Windows向けの軽量画像ビューアーです。画像表示、ズーム、パン、メタデータ表示、別ファイル変換、右クリック保存に対応しています。

## ビルド

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

## 起動

```powershell
.
\build\QuickImageView.exe C:\path\to\image.png
```

Windows Imaging Componentを利用するため、初回ループでは追加の画像処理SDKを必要としません。

## 別ファイル変換

```powershell
.\build\QuickImageView.exe --convert C:\path\to\source.png C:\path\to\output.bmp
```

変換先は必ず別ファイルにします。原本と同じパスや既存ファイルへの上書きは拒否します。

## 方針

- 対応OSはWindows。
- 原本ファイルは変更・削除・上書きしない。
- 変換機能は後続ループで追加する指定形式に限定する。
- 右クリックメニュー登録はインストーラーで選択式にする。

## インストール

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install.ps1 -RegisterContextMenu
```

右クリック登録が不要なら `-RegisterContextMenu` を付けません。詳細は `document/distribution_jp.md` を参照してください。
