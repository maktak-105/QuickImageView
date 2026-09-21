# Development Environment

[日本語版 environment_jp.md](environment_jp.md)

- Windows 10/11 64-bit
- CMake 3.20+
- MinGW-w64 C++17 toolchain
- Python 3.13 for optional test and utility scripts
- Qt 6.10.3 MinGW (`C:\Users\makta\tools\Qt\6.10.3\mingw_64`) and MinGW 13.1.0 (`C:\Users\makta\tools\Qt\Tools\mingw1310_64`) for the Qt Quick target. Override with `QT_ROOT` / `QT_MINGW_BIN` if needed.

Build and test:

```powershell
.\scripts\build.bat
ctest --test-dir build/intermediate/native --output-on-failure
```

Qt Quick:

```powershell
.\scripts\build.bat qt
```

The MinGW runtime is statically linked. The Win32 executable is written to `dist/QuickImageView.exe`. The Qt executable is written to `dist/binary/QuickImageViewQt.exe`.
