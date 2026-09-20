# 開発環境

[English environment.md](environment.md)

- Windows 10/11 64-bit
- CMake 3.20以上
- MinGW-w64 C++17ツールチェーン
- 任意のテスト・補助スクリプト用Python 3.13

ビルドとテスト:

```powershell
.\scripts\build.bat
ctest --test-dir build/intermediate/native --output-on-failure
```

MinGW runtimeは静的リンクしています。実行ファイルは`dist/QuickImageView.exe`へ出力されます。
