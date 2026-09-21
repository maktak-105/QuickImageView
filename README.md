# QuickImageView

QuickImageView is a lightweight Windows image viewer and editor built with Qt 6 (Qt Quick / QML) and C++17. It opens, edits, and converts images, and switches between Japanese and English.

Version: **v4.1.0**

## Download

Download `QuickImageView-binary.zip` and `SHA256SUMS.txt` from [GitHub Releases](https://github.com/maktak-105/QuickImageView/releases). Extract every file into the same folder and run `QuickImageView.exe`. Keep the executable, the Qt DLLs, and the `platforms`, `imageformats`, and `qml` folders together. The ZIP is unsigned and there is no installer.

```powershell
Get-FileHash .\QuickImageView-binary.zip -Algorithm SHA256
```

Compare the result with `SHA256SUMS.txt`.

Version 4 replaces the earlier Win32 implementation. The 3.x releases (up to v3.1.3) stay available at their release tags.

## Features

- Open an image with File > Open image, `Ctrl+O`, drag and drop, or a command-line path; dropping onto a displayed image asks for confirmation
- Fit the image to the window, zoom with the mouse wheel, and pan with a middle-button drag
- Select a region with a left-button drag and crop it from the context menu
- Rotate right, 180 degrees, or left, flip horizontally or vertically, and convert to full color, 256 colors, or grayscale
- Resize by percentage or pixels with an optional aspect-ratio lock
- Undo and redo with `Ctrl+Z` and `Ctrl+Y`
- Copy the image or the selection with `Ctrl+C`; paste an image with `Ctrl+V`, move it, then commit or retry
- Show file name, dimensions, format, file size, and EXIF make, model, and capture date in a floating window that can be copied
- Save as another format with quality or compression options; the source image and existing files are never overwritten, a missing extension is added from the selected file type, and the saved file is reloaded as the current image
- Convert an image from the command line with `--convert`
- Add QuickImageView to the Explorer image context menu, and set the window size (default 720 x 480) in File > Settings
- Switch between Japanese and English with the upper-right button
- Dark theme (including the title bar), in-app Help, and an About dialog

## Supported formats

- Open: BMP, GIF, ICO, JPEG, JPEG XR, PNG, TIFF, Windows Media Photo, and DDS through Windows Imaging Component (WIC); WebP through Qt's WebP image-format plugin; HEIC and HEIF when a compatible WIC codec is installed.
- Save as: PNG, JPEG, BMP, TIFF, WebP, and HEIC/HEIF. TIFF is written through WIC, so no Qt plugin is needed. HEIC/HEIF saving requires a Windows HEIF encoder (HEIF Image Extensions and HEVC Video Extensions) and fails without it.
- Quality (JPEG, WebP, and HEIC/HEIF, 0-100) and compression (PNG 0-9; TIFF 0 = uncompressed, 1-9 = LZW, WIC has no level) can be set before choosing the destination.

## Build

Requirements: Windows 10 or 11 (64-bit), CMake 3.20 or later, Qt 6.10 with the MinGW 13.1.0 toolchain, and Python 3.13 for the helper scripts.

```powershell
.\scripts\build.bat
```

The script configures CMake, builds `QuickImageView.exe` into `dist/`, runs CTest, and deploys the Qt runtime next to the executable. Set `QT_ROOT` and `QT_MINGW_BIN` when Qt is not in the default location. See [docs/environment.md](docs/environment.md).

## Run

```powershell
.\dist\QuickImageView.exe C:\path\to\image.png
```

## Command-line conversion

```powershell
.\dist\QuickImageView.exe --convert C:\path\to\source.png C:\path\to\output.bmp
```

No window is shown. The exit code is 0 when the image was converted and 2 on failure; the reason is written to `crash.log` under `%LOCALAPPDATA%\maktak-105\QuickImageView`. The format follows the destination extension. The source image and existing files are never overwritten.

## Install for the current user

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\install.ps1
```

The script copies the executable and the Qt runtime to `%LOCALAPPDATA%\QuickImageView` and registers the image context menu under the current user (HKCU). Pass `-NoRegisterContextMenu` to skip the registration. Remove it with:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\uninstall.ps1
```

## Create a distribution package

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\package.ps1 -ArchivePath .\build\QuickImageView-binary.zip
```

See [docs/distribution.md](docs/distribution.md).

## Documents

- [docs/spec.md](docs/spec.md): specification
- [docs/environment.md](docs/environment.md): development environment
- [docs/distribution.md](docs/distribution.md): installation and packaging
- [docs/project-structure.md](docs/project-structure.md): folder layout (Japanese)
- [HISTORY.md](HISTORY.md): changelog

## License

QuickImageView is distributed under the MIT License. See [LICENSE](LICENSE). Third-party notices are in [docs/third_party_licenses.md](docs/third_party_licenses.md).
