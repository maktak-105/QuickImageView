# QuickImageView

[日本語版 README](README_jp.md)

Windows向けの軽量画像ビューアー・画像編集アプリです。以下を実装対象とします。

## 表示と基本操作

- コマンドラインまたはファイルメニューから画像を開いて表示する
- 起動後に画像ファイルをウィンドウへドラッグ＆ドロップして開く（画像表示中は確認後に現在の画像を閉じて開く）
- Windowsで利用可能な画像をWIC経由で読み込む
- ウィンドウ内に画像をフィット表示する
- マウスホイールで拡大・縮小する
- 中ドラッグでパンする
- 拡大・縮小・パン中も画像をちらつかせず滑らかに描画する
- 左ドラッグで範囲を選択する
- 選択範囲を表示し、右クリックメニューから切り抜く
- ファイル名、画像寸法、形式、ファイルサイズを表示する
- EXIFが存在する場合、メーカー、機種、撮影日時など代表的な情報を表示する

## リサイズ

- 編集メニューおよび右クリックメニューからリサイズ指定ダイアログを開く
- 任意の正のパーセントを指定する
- 任意の正のピクセル幅・高さを指定する
- アスペクト比固定オプションを表示する
- アスペクト比固定は初期状態でONにする
- リサイズ後は画像データだけでなく、画面上の表示サイズにも結果を反映する
- 拡大表示中にリサイズしても、リサイズ後の寸法変化が画面に反映される

## 画像編集

- 90度、180度、270度回転する
- 左右反転（ミラー）する
- 上下反転する
- フルカラーへ変換する
- 256色へ変換する
- グレースケールへ変換する
- `Ctrl+Z` でUndoする
- `Ctrl+Y` でRedoする
- Undo／Redoをメニュー項目として表示しない
- 編集前の画像状態をUndo用に保持する
- 新しい編集を行った場合、Redo履歴を破棄する

## クリップボード

- 編集メニューから画像をコピーする
- 右クリックメニューから画像をコピーする
- `Ctrl+C` で画像をコピーする
- 編集メニューから画像を貼り付ける
- 右クリックメニューから画像を貼り付ける
- `Ctrl+V` で画像を貼り付ける
- Windowsの画像クリップボード形式を扱う
- `CF_BITMAP`、`CF_DIB`、`CF_DIBV5`を扱う
- 貼り付けた画像を現在の画像として表示する
- 貼り付け前の状態をUndoできる

## 保存と変換

- 原本を変更、削除、上書きしない
- 名前を付けて保存する前に保存オプションを表示する
- 保存先フォルダーの選択前に、保存形式を選択する
- JPEG品質を指定する
- JPEG品質は固定候補だけでなく任意の値を指定できる
- PNG圧縮レベルを指定する
- WebP品質を指定する
- HEIC/HEIF品質を指定する
- 保存形式に応じて使用可能な品質・圧縮オプションだけを有効にする
- 保存形式に応じた拡張子フィルターを表示する
- 拡張子が省略された場合、選択した形式の拡張子を補う
- JPEG、PNG、TIFF、BMP、GIFへ保存・変換する
- WindowsのWICコーデックが利用可能な場合、WebPへ保存・変換する
- WindowsのWICコーデックが利用可能な場合、HEIC/HEIFへ保存・変換する
- 同一パスへの保存を拒否する
- 既存ファイルへの上書きを拒否する
- 変換元ファイルを変更しないことを保証する

コマンドライン変換の形式は次のとおりです。

```powershell
.\build\QuickImageView.exe --convert C:\path\to\source.png C:\path\to\output.bmp
```

## Windows連携

- インストール時に画像ファイルの右クリックメニューへ登録できる
- 右クリックメニュー登録は現在のユーザー（HKCU）に限定する
- 登録コマンドはインストール先のQuickImageView.exeを指す
- Windows 11では「その他のオプションを表示」内から利用できる
- 登録なしでインストールできるオプションを用意する
- アンインストール時はQuickImageView専用の登録だけを削除する
- 他のアプリケーションの登録を削除しない
- インストールした実行ファイルを起動して画像を開ける

## 実装しない機能

以下は対象外です。

- Explorerのサムネイル表示用シェル拡張
- フォルダー内画像の前後移動
- スライドショー
- 印刷
- EXIF・メタデータの編集
- 画像ファイルの原本への直接保存
- 過去の検査結果、作業ログ、ループ履歴の保存

## ビルド環境

- Windows 10またはWindows 11
- CMake 3.20以上
- MinGW-w64 C++17ツールチェーン
- PowerShell 7.6.5以降

WICを利用するため、通常のビルドに追加の画像処理SDKは必要ありません。WebP、HEIC、HEIFはWindowsに対応コーデックがインストールされている場合に利用できます。

## ビルド

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

または、次を実行します。

```powershell
.\build.bat
```

## 起動

```powershell
.\build\QuickImageView.exe C:\path\to\image.png
```

## インストール

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\install.ps1
```

右クリックメニュー登録を行わない場合:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\install.ps1 -NoRegisterContextMenu
```

## アンインストール

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\uninstall.ps1
```

## 配布パッケージ作成

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\package.ps1
```

既定の出力先は `dist/binary/` です。

## ライセンス

MIT Licenseです。詳細は [LICENSE](LICENSE) を参照してください。

WebP保存にはlibwebp 1.6.0を使用します。第三者ライセンスと特許ライセンスは [document/third_party_licenses.md](document/third_party_licenses.md) および `third_party/libwebp-1.6.0/` を参照してください。
