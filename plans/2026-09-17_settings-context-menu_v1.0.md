# 設定画面の追加および右クリックメニューアイコン対応 計画 v1.0

作成日: 2026-09-17

## 目的

1. Windows Explorerの画像右クリックメニューにQuickImageViewのアプリアイコンを表示する。
2. アプリのメニューバー［ファイル］に［設定...］を追加し、設定ダイアログから「右クリックメニューに追加」を選択（有効/無効の切り替え）できるようにする。
3. 配布バイナリのDLL構成を精査し、不要なランタイム（DirectXシェーダーコンパイラ等）を排除して必要最小限のDLL構成にする。

## 実施内容

1. `core/native/qt_app_controller.h` / `qt_app_controller.cpp`:
   - `contextMenuRegistered` プロパティおよび `setContextMenuRegistered` / `isContextMenuRegistered` メソッドを実装。
   - レジストリ（HKCU `SystemFileAssociations\image\shell\QuickImageView`）への `Icon` プロパティ付与、コマンド登録・削除、`SHChangeNotify` によるシェル即時通知を実装。
2. `qml/SettingsDialog.qml`:
   - Quickシリーズのダークテーマ（#111924, #00c9e8）に準拠した中央モーダルダイアログを作成。
   - 「右クリックメニューに追加」の選択式チェックボックスを配置。
3. `qml/Main.qml`:
   - ［ファイル］メニューに［設定...］（`settingsMenuItem`）を追加し、`SettingsDialog` を呼び出し可能にした。
4. `build-tools/deploy_qt.py`:
   - 不要なDLL（`dxcompiler.dll` 26MB、`dxil.dll`、`D3Dcompiler_47.dll`、`Qt6Svg.dll` 等）や不要QMLプラグインを除外し、最小限のDLL構成に縮小。
5. `scripts/install.ps1`:
   - インストール先でも最小限DLL構成が維持されるよう更新。

## 検証計画

- `git diff --check`: PASS
- `build.bat` / `build.bat qt`: PASS
- レジストリへの `Icon` 設定値およびコマンドの確認
- インストール先および `dist/binary/` のDLL一覧・サイズの確認
