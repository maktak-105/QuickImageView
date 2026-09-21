# Explorer右クリックメニューのアイコン表示 計画 v1.0

作成日: 2026-09-17  
対象: `installer/QuickImageView.wxs`、`scripts/install.ps1`

## 目的

Explorerの画像ファイル右クリックメニューにあるQuickImageViewの項目へ、QuickImageView.exeに埋め込まれたアプリアイコンを表示する。

## 実施内容

1. MSIのコンテキストメニュー登録キーへ`Icon`値を追加し、インストール先の専用ICOファイルを指定する。
2. PowerShellインストーラーが専用ICOをインストール先へコピーし、同じ`Icon`値をHKCUへ登録する。
3. 登録先、既定のメニュー名、起動コマンド、任意インストールの仕様は変更しない。

## 検証計画

- WiX XMLとPowerShellを静的に検査し、両経路が同一の実行ファイルとアイコン番号を指定することを確認する。
- `git diff --check`、`build.bat`、`build.bat qt`を実行する。
- WiX Toolsetが利用可能な場合はMSIを生成し、登録値を確認する。
