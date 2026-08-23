# 開発環境

[English environment.md](environment.md)

- Windows 10/11 64-bit
- CMake 3.20以上
- MinGW-w64 C++17ツールチェーン
- Loop検証スクリプト用Python 3

ビルドとテスト:

```powershell
.\build.bat
ctest --test-dir dist/binary --output-on-failure
```

MinGW runtimeは静的リンクしています。`objdump -p dist/binary/QuickImageView.exe`で確認できます。
