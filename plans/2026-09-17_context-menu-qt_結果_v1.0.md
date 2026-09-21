# Qt版右クリック登録の整合化 実施結果 v1.0

実施日: 2026-09-17

## 実施内容

- `scripts/install.ps1 -Qt`を追加し、`QuickImageViewQt.exe`とQtランタイム一式を同じユーザー領域へ配置するようにした。
- Qt指定時のHKCU右クリックコマンドを`QuickImageViewQt.exe`へ登録するようにした。
- 英日配布文書へ`-Qt`の利用方法を追記した。

## 検証結果

- ICOファイルはWindows APIで読み込み成功（32x32、7画像エントリ）。
- 実ユーザー環境の登録先は`QuickImageViewQt.exe`、アイコンは専用`QuickImageView.ico`を指していることを確認した。
- Explorerプロセスを対象PIDだけ再起動し、登録値とアイコンキャッシュを再読み込みさせた。
- Windows 11のモダン右クリックメニューでレジストリ動詞のアイコン表示を保証するには、`IExplorerCommand`とアプリID（MSIXまたはSparse Package）が必要。現行の署名なしZIP/MSI方針のままでは、レジストリ登録は「その他のオプションを表示」の従来メニューが対象となる。
