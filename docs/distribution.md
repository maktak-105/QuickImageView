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

To register the Qt Quick build as the context-menu target and deploy its Qt
runtime, pass `-Qt`:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install.ps1 -Qt
```

If PowerToys Image Resizer, PowerRename, or File Locksmith appears twice in the Windows 11 context menu, run the following once to block only the modern duplicate handlers for the current user. The setting persists across PowerToys and Explorer restarts.

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\cleanup-context-menu.ps1
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
.\scripts\package.ps1 -Qt -OutputDirectory .\build\package-qt -ArchivePath .\build\QuickImageViewQt-v3.1.3-win64.zip
```

The Qt package uses `QuickImageViewQt.exe`, includes the deployed Qt DLLs and
plugins, and keeps the unsigned ZIP distribution model. The MSI continues to
target the stable Win32 executable; the per-user installer switches to the Qt
build when `-Qt` is specified.
