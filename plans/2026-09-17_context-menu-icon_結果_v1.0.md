# Explorer右クリックメニューのアイコン表示 実施結果 v1.0

実施日: 2026-09-17  
対応計画: `plans/2026-09-17_context-menu-icon_v1.0.md`

## 実施内容

- `installer/QuickImageView.wxs`のコンテキストメニュー登録へ`Icon`値を追加し、インストール先の専用ICOファイルを指定した。
- `scripts/install.ps1`でも、専用ICOをコピーしてそのファイルを`Icon`値としてHKCUへ登録するようにした。
- Qt版の配布文書が定めるとおり、MSIとPowerShellインストーラーの起動対象は移行完了までWin32版`QuickImageView.exe`のままとした。Qt版`QuickImageViewQt.exe`への切替は行っていない。
- ユーザー環境の旧HKCU登録（旧`QuickImageView.exe`）を確認し、生成済みQt配布物一式と専用ICOを`%LOCALAPPDATA%\QuickImageView`へ配置したうえで、実際の登録先を`QuickImageViewQt.exe`へ更新した。旧`QuickImageView.exe`は削除していない。

## 検証結果

- `git diff --check`: PASS
- WiX XMLの読み込みと`Icon`値の静的検査: PASS
- PowerShell構文解析とHKCU `Icon`登録処理の静的検査: PASS
- `build.bat`: 実行し、CMake構成とネイティブターゲットのビルド処理開始を確認。Qt SDK未指定の通常経路ではQtを検出せず、従来のWin32ターゲットを使用することを確認した。
- `build.bat qt`: リポジトリ直下のQt 6.10.3 SDKとMinGWを明示指定して実行。`dist/binary/QuickImageViewQt.exe`とQtランタイムを生成した。
- 実配置後の`QuickImageViewQt.exe --self-test`: PASS。Explorerへ関連付け変更通知を送信した。
- Qtテスト: `smoke_self_test`および`qiv_qml_tests`はPASS。`qiv_controller_tests`はCTestで15秒タイムアウトし、`-platform offscreen`の単独実行でも`std::system_error: Invalid argument`で終了した。今回のインストーラー登録変更とは独立した既存のQtテスト不具合として未解決。
- WiX Toolsetがこの環境にないため、MSI生成による最終検証は未実施。
- Explorerの実画面での右クリックメニュー表示は、ユーザー環境のHKCU登録を変更しないため未実施。
