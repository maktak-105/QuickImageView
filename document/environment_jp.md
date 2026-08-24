# 開発環境

[English environment.md](environment.md)

- Windows 10/11 64-bit
- CMake 3.20以上
- MinGW-w64 C++17ツールチェーン
- 任意のテスト・補助スクリプト用Python 3.13
- Qt Quick版は Qt 6.10.3 MinGW（`C:\Users\makta\tools\Qt\6.10.3\mingw_64`）と MinGW 13.1.0（`C:\Users\makta\tools\Qt\Tools\mingw1310_64`）。必要なら `QT_ROOT` / `QT_MINGW_BIN` で上書きする。

ビルドとテスト:

```powershell
.\build.bat
ctest --test-dir build/native --output-on-failure
```

Qt Quick:

```powershell
.\build.bat qt
```

MinGW runtimeは静的リンクしています。Win32実行ファイルは`dist/binary/QuickImageView.exe`、Qt実行ファイルは`dist/binary/QuickImageViewQt.exe`へ出力されます。
