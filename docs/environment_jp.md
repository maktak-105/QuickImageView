# 開発環境

[English environment.md](environment.md)

- Windows 10/11 64-bit
- CMake 3.20以上
- Qt 6.10.3 MinGW（`C:\Users\makta\tools\Qt\6.10.3\mingw_64`）と MinGW 13.1.0（`C:\Users\makta\tools\Qt\Tools\mingw1310_64`）。別の場所にある場合は `QT_ROOT` / `QT_MINGW_BIN` で上書きする。
- ビルド・デプロイスクリプト用のPython 3.13
- `aqtinstall`（`scripts/requirements.txt`）は、Qt SDKをローカルに導入するための任意ツール。

ビルドとテスト:

```powershell
.\scripts\build.bat
```

`build/intermediate/qt-mingw1310`でCMakeを構成し、`QuickImageViewQt`、`qiv_controller_tests`、`qiv_qml_tests`をビルドしてCTestを実行し、`windeployqt`（`scripts/deploy_qt.py`）でQtランタイムを配置します。実行ファイルは`dist/QuickImageViewQt.exe`へ出力され、Qtランタイムはその横へ置かれます。

GitHub Actionsは`windows-2022`上で、Qt 6.10.3 MSVC 2022（`win64_msvc2022_64`）を使ってビルドします。
