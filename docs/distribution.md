# QuickImageView Distribution

[日本語版 distribution_jp.md](distribution_jp.md)

QuickImageView is distributed as an unsigned ZIP on GitHub Releases. There is no installer. Each release attaches `QuickImageView-binary.zip` and a CI-generated `SHA256SUMS.txt`.

The ZIP contains `QuickImageView.exe`, the deployed Qt DLLs and plugins (`platforms`, `imageformats`, `qml`), the bilingual README and history, the MIT License, the Japanese license notice, the third-party notices, and the libwebp COPYING/PATENTS files. The bilingual help is embedded in the executable.

## Build

```powershell
.\scripts\build.bat
```

See [environment.md](environment.md) for the toolchain.

## Package

```powershell
.\scripts\package.ps1 -OutputDirectory .\build\intermediate\package -ArchivePath .\build\QuickImageView-binary.zip
```

`package.ps1` collects the executable and the Qt runtime from `dist/` and the distribution documents from `docs/distribution/`. Run `build.bat` first so that `dist/` holds the deployed runtime.

## Release

Pushing a `v*` tag runs `.github/workflows/release.yml`, which builds with Qt 6.10.3 (MSVC 2022), runs CTest, deploys the Qt runtime, creates the ZIP and `SHA256SUMS.txt`, and publishes them to the release. The workflow can also be started manually with an existing tag.

## Install

The PowerShell installer copies the executable and the Qt runtime to the current user's LocalAppData directory and registers the image context-menu entry under the current user (HKCU). It does not require administrator privileges.

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install.ps1
```

To install without the registration:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install.ps1 -NoRegisterContextMenu
```

If PowerToys Image Resizer, PowerRename, or File Locksmith appears twice in the Windows 11 context menu, run the following once to block only the modern duplicate handlers for the current user. The setting persists across PowerToys and Explorer restarts.

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\cleanup-context-menu.ps1
```

## Uninstall

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\uninstall.ps1
```

The uninstaller removes the QuickImageView per-user application directory and its own context-menu registration. It does not remove user-created image files.
