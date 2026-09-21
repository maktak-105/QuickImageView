# QuickImageView

Qt 6（Qt Quick / QML）とC++17で作ったWindows向けの軽量画像ビューアー・画像編集アプリです。画像の表示、編集、別形式保存ができ、日本語と英語を切り替えられます。

バージョン: **v4.2.0**

## 配布版を使う

[GitHub Releases](https://github.com/maktak-105/QuickImageView/releases)から`QuickImageView-binary.zip`と`SHA256SUMS.txt`を取得します。ZIPの中身をすべて同じフォルダーへ展開し、`QuickImageView.exe`を実行してください。実行ファイル、Qt DLL、`platforms`・`imageformats`・`qml`フォルダーは同じ場所に置く必要があります。ZIPは署名なしで、インストーラーはありません。

```powershell
Get-FileHash .\QuickImageView-binary.zip -Algorithm SHA256
```

結果を`SHA256SUMS.txt`と照合してください。

バージョン4で、従来のWin32実装からQt Quick実装へ切り替えました。3.x系（v3.1.3まで）は各リリースのタグから取得できます。

## 機能

- ファイルメニュー、`Ctrl+O`、ドラッグ＆ドロップ、コマンドライン引数で画像を開く（画像表示中のドロップは確認を出す）
- ウィンドウへのフィット表示、マウスホイールで拡大・縮小、中ドラッグでパン
- 左ドラッグで範囲を選択し、右クリックメニューから切り抜く
- 右90度・180度・左90度の回転、左右反転、上下反転、フルカラー・256色・グレースケールへの色変換
- パーセントまたはピクセルでのリサイズ（縦横比の保持を選択可能）
- `Ctrl+Z`でUndo、`Ctrl+Y`でRedo
- `Ctrl+C`で画像または選択範囲をコピー、`Ctrl+V`で貼り付け、移動してから確定またはやり直し
- ファイル名、寸法、形式、ファイルサイズ、EXIFのメーカー・機種・撮影日時をフローティングウィンドウに表示し、コピーできる
- 品質・圧縮オプションを指定して別形式で保存する。ファイル選択は画像のあるフォルダーで開き、原本と既存ファイルは上書きせず（警告ダイアログで知らせる）、拡張子が無ければ選択した形式の拡張子を補い、保存したファイルを現在の画像として読み込み直す
- `--convert`でコマンドラインから画像を変換する
- ファイル > 設定から、開ける全ての画像（HEIC・HEIF・WebPを含む）のExplorer右クリックメニューへの登録と、ウィンドウサイズ（既定は720×480）の指定ができる
- 右上のボタンで日本語とEnglishを切り替える
- ダークテーマ（タイトルバーを含む）、アプリ内ヘルプ、バージョン情報

## 対応形式

- 開く: BMP、GIF、ICO、JPEG、JPEG XR、PNG、TIFF、Windows Media Photo、DDSはWindows Imaging Component（WIC）で読み込みます。WebPはQtのWebP画像フォーマットプラグインで読み込みます。HEICとHEIFは、対応するWICコーデックが導入されている場合に読み込めます。
- 保存: PNG、JPEG、BMP、TIFF、WebP、HEIC/HEIFに保存できます。TIFFはWICで書くため、Qtのプラグインは不要です。HEIC/HEIFの保存にはWindowsのHEIFエンコーダー（HEIF画像拡張機能とHEVCビデオ拡張機能）が必要で、無い環境では保存に失敗します。
- 保存先を選ぶ前に、品質（JPEG・WebP・HEIC/HEIF、0〜100）と圧縮（PNGは0〜9、TIFFは0＝圧縮なし・1〜9＝LZW。WICには段階指定がありません）を指定できます。

## ビルド

必要なもの: Windows 10/11（64bit）、CMake 3.20以上、Qt 6.10とMinGW 13.1.0、補助スクリプト用のPython 3.13。

```powershell
.\scripts\build.bat
```

CMakeの構成、`dist/`への`QuickImageView.exe`のビルド、CTest、実行ファイル横へのQtランタイム配置までを行います。Qtが既定の場所にない場合は`QT_ROOT`と`QT_MINGW_BIN`を設定してください。詳細は[docs/environment_jp.md](docs/environment_jp.md)を参照してください。

## 実行

```powershell
.\dist\QuickImageView.exe C:\path\to\image.png
```

## コマンドラインでの変換

```powershell
.\dist\QuickImageView.exe --convert C:\path\to\source.png C:\path\to\output.bmp
```

ウィンドウは表示しません。変換できたときの終了コードは0、失敗時は2で、理由は`%LOCALAPPDATA%\maktak-105\QuickImageView`の`crash.log`に書き込みます。形式は出力先の拡張子で決まります。原本と既存ファイルは上書きしません。

## 現在のユーザーへインストール

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\install.ps1
```

実行ファイルとQtランタイムを`%LOCALAPPDATA%\QuickImageView`へ配置し、現在のユーザー（HKCU）の、全ての画像の右クリックメニューへ登録します。登録しない場合は`-NoRegisterContextMenu`を指定します。削除は次のとおりです。

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\uninstall.ps1
```

## 配布パッケージを作る

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\package.ps1 -ArchivePath .\build\QuickImageView-binary.zip
```

詳細は[docs/distribution_jp.md](docs/distribution_jp.md)を参照してください。

## 文書

- [docs/spec_jp.md](docs/spec_jp.md): 仕様書
- [docs/environment_jp.md](docs/environment_jp.md): 開発環境
- [docs/distribution_jp.md](docs/distribution_jp.md): インストールと配布
- [docs/project-structure.md](docs/project-structure.md): フォルダ構成
- [HISTORY_jp.md](HISTORY_jp.md): 変更履歴

## ライセンス

QuickImageViewはMIT Licenseで配布します。[LICENSE](LICENSE)を参照してください。第三者通知は[docs/third_party_licenses_jp.md](docs/third_party_licenses_jp.md)にあります。
