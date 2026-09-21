QuickImageView - 配布パッケージ v4.0.0

この文書は配布物の内容と、このバージョンで文書化されている動作を説明するものです。

動作環境
--------
- Windows 10 / 11 (64-bit)
- Windows Imaging Component（Windows標準）

使い方
------
起動と画像を開く:
- ZIPの中身をすべて同じフォルダーへ展開し、QuickImageView.exeを実行します。
  実行ファイル、Qt DLL、platforms・imageformats・qmlフォルダーは同じ場所に置きます。
- 「ファイル > 画像を開く」またはCtrl+Oを使います。画像をウィンドウへドラッグして
  開くことも、コマンドラインで画像パスを渡すこともできます。
- 原本画像は上書きしません。

表示と編集:
- 画像を開くとウィンドウに合わせて表示します。マウスホイールで拡大・縮小し、
  中ドラッグで移動します。
- 左ドラッグで矩形範囲を選択し、右クリックメニューから切り抜きます。
- 回転、反転、色変換、リサイズ（パーセントまたはピクセル、縦横比の保持を選択可能）を
  右クリックメニューから実行できます。
- Ctrl+Z / Ctrl+YでUndo / Redoします。

コピーと貼り付け:
- Ctrl+Cで画像または選択範囲をコピーし、Ctrl+Vで画像を貼り付けます。
- 貼り付け画像は移動でき、右クリックメニューで確定またはやり直します。

EXIFと保存:
- ファイル情報とEXIF情報はフローティングウィンドウに表示します。EXIFテキストは
  コピーできます。
- 「ファイル > 別形式で保存」は、ファイル選択の前に保存オプションを表示します。
  品質（JPEGとWebP、0〜100）と圧縮（PNGとTIFF、0〜9）を指定できます。
- 既存ファイルと原本画像は上書きしません。

対応形式とコーデック:
- 開く: BMP、GIF、ICO、JPEG、JPEG XR、PNG、TIFF、Windows Media Photo、DDSは
  Windows Imaging Component（WIC）を使用します。WebPはQtのWebP画像フォーマット
  プラグインを使用します。HEICとHEIFは、PCに対応するWICコーデックが必要です。
- 保存: PNG、JPEG、BMP、WebPに保存できます。TIFFの保存にはQtのTIFF画像プラグイン、
  HEIC/HEIFの保存にはWICエンコーダーが必要で、ない場合は保存に失敗します。

言語、テーマ、Windows連携:
- 右上のボタンは水色の地球儀とEnglishまたは日本語を表示します。押して切り替えます。
- メニューは「ファイル、編集、ヘルプ」の順です。「ヘルプ > ヘルプ」で同梱ヘルプを開き、
  「ヘルプ > バージョン情報」で版数と作者情報を確認できます。
- 「ファイル > 設定」で、Explorerの画像右クリックメニューへQuickImageViewを追加・削除
  できます（現在のユーザー、HKCU）。

配布ファイル
------------
- QuickImageView.exeとQtランタイム（Qt6*.dll、platforms、imageformats、qmlなど）
- C++ランタイムDLL（vcruntime140*.dll、msvcp140*.dll）。Visual C++再頒布可能パッケージの別途導入は不要です
- readme.txt / readme_jp.txt
- history.txt / history_jp.txt
- LICENSE.txt / LICENSE_jp.txt
- third_party_licenses.md / third_party_licenses_jp.md
- libwebp-COPYING / libwebp-PATENTS
- ヘルプはQuickImageView.exeへ英語・日本語で埋め込まれています。

SHA-256
-------
GitHub Releasesには、CIが生成したZIPのチェックサムSHA256SUMS.txtを添付します。
https://github.com/maktak-105/QuickImageView/releases
ZIPの確認: Get-FileHash .\QuickImageView-binary.zip -Algorithm SHA256

ライセンス
----------
MIT License。LICENSE.txtを確認してください。QuickImageViewはQt 6を動的ライブラリとして使用します。
詳細はthird_party_licenses_jp.mdを参照してください。WebP対応はQtのWebP画像フォーマット
プラグインによるもので、このプラグインはlibwebpを含んでいます。libwebpのBSD系ライセンスと
特許通知はlibwebp-COPYING、libwebp-PATENTSとして提供します。
