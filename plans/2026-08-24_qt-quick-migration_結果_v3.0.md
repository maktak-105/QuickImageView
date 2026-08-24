# QuickImageView v3.1.2 Qt Quick移行 実施結果 v3.0

対象ブランチ：`codex/qt-quick-migration`
対象：前任者から引き継いだQt Quick版の不具合修正

## 発端

引継ぎ資料 v3.0 では「主要未実装機能を実装完了」「CTest 3件すべてPASS」と記載されていたが、
実機では **画面は表示されるがメニュー・右クリックが一切反応しない** 状態だった。
自動テストが通ることと実際に動作することが乖離していた。

## 原因調査で判明した事実

- `tests/qml/` のQMLテストは、ダイアログが開くかどうかを見るだけの薄いスモークテストで、
  メニュー操作やクリック操作を一切検証していなかった。
- ビルドフォルダに `qiv_qml_interaction_tests.exe` という、現行 `CMakeLists.txt` に存在しない
  テストの実行ファイルが残骸として残っていた。
- 操作不能の直接原因は `Menu` の `background` を `implicitWidth` 未指定のカスタム `Rectangle` に
  差し替えていたこと。Qt標準の `Menu.qml` ではメニュー全体の幅が実質 `background` の
  `implicitWidth` のみで決まるため、メニュー幅が潰れて項目が描画されず、クリックも当たらなかった。
  最小再現QMLで、この行を削除すると正常動作することを確認した。

## 修正内容

### 操作不能

- `Menu` の `background` に `implicitWidth` / `implicitHeight` を明示。
- 自前の `ToolBar` + `ToolButton` + `Menu.popup()` 構成を、標準の `MenuBar` 構成へ置換。
- 定義だけされて未使用だった `DarkMenuItem` / `DarkMenuBarItem` を本来の用途で使用。

### 挙動の是正（正本はWin32版 `core/native/main.cpp`）

- **リサイズがでたらめ・ボケる**：正本の初期値 `100` は「パーセント」であるのに、Qt版は
  「ピクセル」として移植していた（`main.cpp:1862-1869`）。100×100pxへ潰れた画像を画面一杯に
  拡大表示するため「ボケる」症状も併発していた。単位選択（パーセント／ピクセル）と
  縦横比保持を正本と同じ順序で実装。
- **リサイズしても見た目が変わらない**：正本 `ResizeCurrentImageForDisplay`（`main.cpp:1118`）は
  リサイズ前後で表示縮尺を維持するため `zoom` を逆算している。Qt版はこれが欠落し、
  `fitScale` の再計算で相殺されて見た目が変化しなかった。同ロジックを移植（クランプ 0.1〜20倍）。
- **範囲選択で即切り抜き**：選択終了時に `cropImage()` を直接呼んでいた。選択は保持のみとし、
  右クリックメニューの「選択範囲を切り抜く」で確定する方式へ変更（正本 `kCommandCrop` と同じ）。
  4ピクセル以下のドラッグは選択解除扱い（`main.cpp:2630` と同じ閾値）。
- **選択範囲コピー**：`copyImageRegion()` を追加し、選択中はその範囲のみクリップボードへ
  （正本 `CopyImageToClipboard` の `g_selectionActive` 分岐と同じ）。
- **メニュー構成**：編集メニューはUndo/Redoのみ。リサイズ・色変換・回転・反転は右クリックへ集約。
- **ホイール拡大縮小／中ボタンパン**：Qt版に未実装だった。正本と同じ 1段あたり1.15倍・
  カーソル位置固定のズームと、中ボタンドラッグによるパン、範囲クランプを実装。
- **アイコン**：Qt版に `.rc` リソースが存在しなかった。`core/native/qt_resource.rc` を新規作成し
  `CMakeLists.txt` へ追加。

### 追加要望

- **ファイル情報／EXIF情報のフローティングウィンドウ常時表示**：`qml/InfoWindow.qml` を新規作成。
  正本 `PlaceExifWindow`（`main.cpp:652`）と同じく本体の右隣へ12px間隔で配置し、画面外へ
  はみ出す場合は左へ回り込む。`Qt.Tool` でタスクバーに出さず前面維持。
  ファイル名／画像サイズ／ファイルサイズ／形式／場所とEXIFを表示し、`fileInfoText` プロパティと
  `fileInfoChanged` シグナルで自動追従。従来のモーダルEXIFダイアログは廃止。
- **貼り付けドラッグの追従改善**：ドラッグ用 `MouseArea` が移動する画像自身の子であったため、
  画像が動くとローカル座標の基準もずれて自己干渉していた。静止した `viewport` 座標系へ
  `mapToItem` で変換して差分を取るよう修正。あわせて `movePaste()` が位置無変化でも
  シグナルを発行していたのを、変化時のみ通知するよう変更。

## 検証

- `build.bat qt`：成功（エラー0件）。
- CTest：`smoke_self_test`、`qiv_controller_tests`、`qiv_qml_tests` の3件すべてPASS。
- `qmllint`：`Main.qml`・`InfoWindow.qml` ともに実害のある指摘は0件。
  残る警告はすべて `[unqualified]` で、`appController` をC++の
  `setContextProperty` で注入しているために静的解決できないもの。

## 未解決・次の担当者への申し送り

- **「メニュー項目の選択がどんどん遅くなる」の根本原因は未特定**。毎回 `Menu.popup()` で
  ポップアップを生成し直す構成が疑わしいと判断して標準 `MenuBar` へ置換したが、
  これで解消するかは実機確認が必要。解消しない場合は別原因として再調査すること。
- **UIの動作確認は未実施**。自動テストは画面操作を検証していない。上記すべての挙動は
  実機での目視確認が必要。
- `ImageEngine::exifText()` はEXIFの項目名（「メーカー=」等）を言語設定に関わらず日本語で
  返している。正本 `ExifText()`（`main.cpp:207`）は英日を切り替えるため、挙動が一致していない。
- Qt版はlibwebpに依存せずQtの `qwebp.dll` プラグインでWebPを読み書きする
  （`CMakeLists.txt` のQt版ターゲットは `webp` をリンクしていない）。一方
  `scripts/package.ps1` が `dist/documents` を一括コピーするため、Qt版の配布ZIPにも
  libwebpのライセンスファイルが同梱される。正式配布へ切り替える際に整理が必要。
- `6.10.3/`（約1.2GBの重複Qt SDK）は誤コミット防止のため `.gitignore` へ追加した。
  本来は削除するか、リポジトリ外へ移すのが望ましい。
