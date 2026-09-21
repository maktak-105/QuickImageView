# 右クリックの重複登録整理 実施結果 v1.0

実施日: 2026-09-17

## 実施内容

- 旧`QuickImageView.exe`の`.jpg`／`.png`の`OpenWithList`登録を削除した。
- 専用右クリック動詞と重複していた`QuickImageViewQt.exe`の`.png` `OpenWithList`登録を削除した。
- `Antigravity.exe`を現行`Antigravity IDE.exe`へ置換し、旧実行ファイルのアプリ登録を削除した。
- 実体がなく現行版と重複していた`QuickMarkPDF_webview.exe`の`.pdf`登録を削除した。
- 画像の既定アプリ（`UserChoice`）と他社アプリの有効な登録は変更していない。

## 検証結果

- 全拡張子の`OpenWithList`を再走査し、同一拡張子内の同一アプリ重複: なし。
- QuickImageViewの`OpenWithList`登録: なし（専用右クリック動詞のみ）。
- QuickImageViewの専用動詞: Qt版実行ファイル＋専用ICOを指すことを確認。
- Explorerを再起動し、登録変更を反映した。

## 追加対応（Windows 11のモダンメニュー重複）

- スクリーンショットの重複はQuickImageViewではなく、PowerToysのImage Resizer／PowerRenameがモダン用と従来用を二重登録しているものと特定した。
- 原因はPowerToysがImage Resizer／PowerRename／File Locksmithを、従来COMハンドラーとWindows 11 sparse MSIXハンドラーの両方で登録していることだった。前者だけを消しても、PowerToysやExplorerの再登録で復活する。
- `scripts/cleanup-context-menu.ps1`を追加し、モダン側3 CLSIDを`HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Blocked`へ登録する永続的な対処に変更した。従来側の項目を1件残すため、機能は維持される。
- 旧仕様を維持するユーザー要望に合わせ、`CLSID\\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}\\InprocServer32`の空値は復元した。Windows 11 24H2では無視される場合があるため、Explorer再起動後の実画面確認が必要。
- Explorerを対象PIDだけ再起動し、ブロック値を再読込させた。
- フォト／ペイント／Clipchampの上段項目はWindowsシェルが生成するパッケージアクションであり、QuickImageViewの登録から「プログラムから開く」へ移動できる種類ではない。PowerToysの重複とは分離して扱った。

## 画像アセット

- 削除されていた`assets/maktak105-V04-01.jpg`は、同一内容を保持していた`QuickRightClick/assets/maktak105-V04-01.jpg`から復元した。Git blobハッシュ一致と目視確認で検証した。

## ビルド状況

- アセット復元後に`build.bat`／`build.bat qt`を再実行し、結果をこの文書へ追記する。
