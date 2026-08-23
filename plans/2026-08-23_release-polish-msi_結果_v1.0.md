# リリース整備・日英UI・MSI 実施結果 v1.0

実施日: 2026-08-23  
対応する計画書: `plans/2026-08-23_release-polish-msi_v1.0.md`

## 1. 実施内容

- 黒基調の画面にSegoe UIを適用し、右上の`EN`/`日本語`ボタンでメニューとEXIF表示を切り替えられるようにした。
- 実行時生成のアプリアイコンと、QuickImageView 1.0.0の実行ファイルバージョン情報を追加した。
- 配布用の日英説明書、履歴、MIT License、libwebpのCOPYINGとPATENTSをパッケージ処理へ統合した。
- WICの標準・追加コーデックによる対応形式を日英仕様書へ明記した。
- WiX MSI定義を追加した。右クリックメニュー、JPEG、PNG、TIFF、BMP/GIF、WebP、HEIC/HEIFの関連付けは全て既定で未選択の機能である。

## 2. 検証結果

- Releaseビルド: PASS
- `ctest --test-dir build --output-on-failure`: 1/1 PASS
- 実UI: `EN`ボタンの存在、クリック後の`日本語`表示への変更を確認
- 配布フォルダ: 実行ファイル、日英README、日英履歴、MIT License、libwebpのCOPYING/PATENTS、MSI用LICENSE.rtfを確認
- Quickアプリ整備: `assets/QuickImageView-icon.svg`、日英ヘルプ、`.gitattributes`を追加し、言語切替の実UIテスト（feature_015）をPASS

## 3. 残課題

- WiX 3.14.1で`installer/build-msi.ps1`を実行し、`dist/QuickImageView-1.0.0-x64.msi`（827,392 bytes）を生成した。WiXの非推奨警告はレジストリキーの削除指定に関するもので、MSIの生成は成功している。
