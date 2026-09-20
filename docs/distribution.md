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
cmake -S . -B build/intermediate/native -G "MinGW Makefiles"
cmake --build build/intermediate/native --parallel 2
ctest --test-dir build/intermediate/native --output-on-failure
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
