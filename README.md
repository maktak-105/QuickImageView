# QuickImageView

[日本語版 README_jp.md](README_jp.md)

Windows向けの軽量画像ビューアーです。画像表示、ズーム、パン、画像編集、別ファイル変換、右クリック保存に対応しています。

編集操作:

- `Ctrl+1`〜`Ctrl+4`: 50%、75%、125%、200%リサイズ
- 編集メニュー「リサイズを指定...」: パーセントまたはピクセルで幅・高さを指定
- 左ドラッグ: 範囲選択、右クリックから切り抜き（中ドラッグはパン）
- `Ctrl+Z` / `Ctrl+Y`: Undo / Redo（メニュー項目なし）
- 右クリック: 回転、ミラー（左右反転 / Mirror）、上下反転、色変換、JPEG品質、PNG圧縮
- 編集メニュー: 画像のクリップボードコピー・貼り付け
- 保存形式: JPEG、PNG、TIFF、BMP、GIF、WebP、HEIC/HEIF（WindowsのWICコーデックが必要）

## ビルド

必要環境:

- Windows 10/11
- CMake 3.20以上
- MinGW-w64（C++17対応）
- PowerShell 5.1以降（完了判定用）

このリポジトリの既定ビルドは `MinGW Makefiles` です。CMakeがコンパイラを見つけられない場合は、MinGW-w64の `bin` を `PATH` に追加してから構成してください。

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

完了判定:

```powershell
powershell -ExecutionPolicy Bypass -File .\tests\verify_goal.ps1
```

終了コード0がMVP完了です。完了条件は `docs/loop/goal.json`、検査本体は `tests/verify_goal.ps1` が正本です。過去のループログや証跡ファイルは判定に使用しません。

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

変換対応形式は JPG、PNG、TIFF、BMP、GIF、WebP、HEIC/HEIF です。WebP/HEIC/HEIFはWindows側のWICコーデックが必要です。

## 方針

- 対応OSはWindows。
- 原本ファイルは変更・削除・上書きしない。
- 変換機能は後続ループで追加する指定形式に限定する。
- 右クリックメニュー登録はインストーラーで選択式にする。

## インストール

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install.ps1
```

右クリック登録が不要なら `-NoRegisterContextMenu` を付けます。Windows 11では「その他のオプションを表示」の中に表示されます。詳細は `document/distribution_jp.md` を参照してください。
