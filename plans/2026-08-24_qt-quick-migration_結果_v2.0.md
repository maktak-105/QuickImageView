# QuickImageView v3.1.2 Qt Quick移行 実施結果 v2.0

対象ブランチ：`codex/qt-quick-migration`  
対象：前任者から引き継いだQt Quick版の未実装機能

## 実装内容

- 画像上の左ドラッグによる範囲選択と切り抜き。
- クリップボード画像の仮配置、画像内移動、範囲外への移動制限、確定、やり直し。
- 保存前のJPEG/WebP品質、PNG/TIFF圧縮設定。
- WICメタデータクエリによるEXIFメーカー・機種・撮影日時取得と、EXIFなし表示・コピー。
- Windows WIC HEIC/HEIFエンコーダーを利用した保存フォールバック。
- コマンドライン引数で指定された画像の起動時読込。
- Qt診断メッセージと未処理Windows例外の`crash.log`記録。
- 画像ファイルのドラッグ＆ドロップ読込。
- Qt Quick版のDLL・プラグイン・QMLモジュールを含む署名なしZIP作成オプション。
- QML主要操作に固定`objectName`/`Accessible.name`を付与し、切り抜き・貼り付け・保存設定の検査対象を追加。

## 検証

- `git diff --check`：成功。
- `build.bat`：成功。従来Win32ターゲットをビルド。
- `build.bat qt`：成功。
- CTest：`smoke_self_test`、`qiv_controller_tests`、`qiv_qml_tests` の3件すべてPASS。
- C++テスト：切り抜き、貼り付け仮配置・確定、保存設定、WIC読込、WebP読込、回転、色変換、Undo/Redoを検証。
- Qt配布フォルダ起動：Qtの`bin`をPATHへ追加しない状態で起動確認。対象の`QuickImageViewQt.exe`プロセスだけをPID指定で終了。
- Qt ZIP：`build/QuickImageViewQt-v3.1.2-win64.zip`を作成し、Qt6Core.dll、`platforms/qwindows.dll`、Qt実行ファイルを確認。旧`QuickImageView.exe`は含めない。

## 残る実機確認

- EXIF付きJPEG、HEIC/HEIF WICコーデックの実機fixture確認。
- 高DPI倍率別の目視確認とスクリーンリーダー実機確認。
- Explorerの右クリック関連付けをQt版実行ファイルへ切り替える最終判断。現行インストーラーは安定版Win32版を対象とする計画を維持している。
