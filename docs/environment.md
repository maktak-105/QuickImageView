# Development Environment

[日本語版 environment_jp.md](environment_jp.md)

- Windows 10/11 64-bit
- CMake 3.20+
- Qt 6.10.3 MinGW (`C:\Users\makta\tools\Qt\6.10.3\mingw_64`) and MinGW 13.1.0 (`C:\Users\makta\tools\Qt\Tools\mingw1310_64`). Override with `QT_ROOT` / `QT_MINGW_BIN` if Qt is installed elsewhere.
- Python 3.13 for the build and deploy scripts
- `aqtinstall` (`scripts/requirements.txt`) is an optional tool for provisioning the Qt SDK locally.

Build and test:

```powershell
.\scripts\build.bat
```

The script configures CMake in `build/intermediate/qt-mingw1310`, builds `QuickImageView`, `qiv_controller_tests`, and `qiv_qml_tests`, runs CTest, and deploys the Qt runtime with `windeployqt` (`scripts/deploy.py`). The executable is written to `dist/QuickImageView.exe` and the Qt runtime is placed next to it.

GitHub Actions builds with Qt 6.10.3 for MSVC 2022 (`win64_msvc2022_64`) on `windows-2022`.
