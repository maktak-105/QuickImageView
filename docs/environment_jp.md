# 開発環境

[English environment.md](environment.md)

- Windows 10/11 64-bit
- CMake 3.20以上
- Qt 6.10.3 MinGW（`%USERPROFILE%\tools\Qt\6.10.3\mingw_64`）と MinGW 13.1.0（`%USERPROFILE%\tools\Qt\Tools\mingw1310_64`）。別の場所にある場合は `QT_ROOT` / `QT_MINGW_BIN` で上書きする。
- ビルド・デプロイスクリプト用のPython 3.13
- `aqtinstall`（`scripts/requirements.txt`）は、Qt SDKをローカルに導入するための任意ツール。

ビルドとテスト:

```powershell
.\scripts\build.bat
```

`build/intermediate/qt-mingw1310`でCMakeを構成し、`QuickImageView`、`qiv_controller_tests`、`qiv_qml_tests`をビルドしてCTestを実行し、`windeployqt`（`scripts/deploy.py`）でQtランタイムを配置します。実行ファイルは`dist/QuickImageView.exe`へ出力され、Qtランタイムはその横へ置かれます。

GitHub Actionsは、ローカルと同じ構成（Qt 6.10.3 MinGW〔`win64_mingw`〕とMinGW 13.1.0〔`tools_mingw1310`〕）を`windows-2022`上に導入し、`scripts/build.py`を実行してビルドします。
