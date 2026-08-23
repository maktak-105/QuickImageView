# QuickImageView Distribution

[日本語版 distribution_jp.md](distribution_jp.md)

This document describes the current source-tree distribution procedure. It does not
mark the 72 baseline requirements or any additional request as UI-tested.

The ZIP and MSI distribution files include the executable, bilingual README,
history, help, MIT License, Japanese license notice, and libwebp COPYING/PATENTS.
The MSI exposes the Explorer context menu and each supported image extension as
independent optional features; all are unselected by default. The PowerShell
installer remains a per-user HKCU installation.

## Build

```powershell
cmake -S . -B dist/binary -G "MinGW Makefiles"
cmake --build dist/binary --parallel 2
ctest --test-dir dist/binary --output-on-failure
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
