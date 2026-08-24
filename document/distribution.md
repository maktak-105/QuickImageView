# QuickImageView Distribution

[日本語版 distribution_jp.md](distribution_jp.md)

This document describes how to build, install, and remove the current QuickImageView distribution.

The ZIP and MSI distribution files include the executable with embedded bilingual
help, bilingual README, history, MIT License, Japanese license notice, and
libwebp COPYING/PATENTS.
The MSI exposes the Explorer context menu and each supported image extension as
independent optional features; all are unselected by default. The PowerShell
installer remains a per-user HKCU installation.

## Build

```powershell
cmake -S . -B build/native -G "MinGW Makefiles"
cmake --build build/native --parallel 2
ctest --test-dir build/native --output-on-failure
```

## Install

The PowerShell installer copies the application executable to the current user's
LocalAppData directory.

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install.ps1
```

The default installation registers the QuickImageView image context-menu entry
under the current user (HKCU). To install without that registration, use:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install.ps1 -NoRegisterContextMenu
```

The installer does not require administrator privileges for this per-user setup.

## Uninstall

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\uninstall.ps1
```

The uninstaller removes the QuickImageView per-user application directory and its
own context-menu registration. It does not remove user-created image files.

## Qt Quick preview ZIP

After `build.bat qt`, package the Qt Quick executable and its deployed Qt
runtime without the legacy executable:

```powershell
.\scripts\package.ps1 -Qt -OutputDirectory .\build\package-qt -ArchivePath .\build\QuickImageViewQt-v3.1.2-win64.zip
```

The Qt package uses `QuickImageViewQt.exe`, includes the deployed Qt DLLs and
plugins, and keeps the unsigned ZIP distribution model. The existing MSI and
per-user installer continue to target the stable Win32 executable until the
Qt migration replaces that target deliberately.
