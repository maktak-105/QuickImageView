# Development Environment

[日本語版 environment_jp.md](environment_jp.md)

- Windows 10/11 64-bit
- CMake 3.20+
- MinGW-w64 C++17 toolchain
- Python 3.13 for optional test and utility scripts

Build and test:

```powershell
.\build.bat
ctest --test-dir build/native --output-on-failure
```

The MinGW runtime is statically linked. The executable is written to `dist/binary/QuickImageView.exe`.
