QuickImageView - Distribution package v4.0.0

This document describes the package contents and the documented behavior of
this version.

Requirements
------------
- Windows 10 / 11 (64-bit)
- Windows Imaging Component (included with Windows)

Usage
-----
Starting and opening images:
- Extract every file of the ZIP into the same folder and run
  QuickImageView.exe. Keep the executable, the Qt DLLs, and the platforms,
  imageformats, and qml folders together.
- Use File > Open image or Ctrl+O, drag an image onto the window, or pass an
  image path on the command line.
- The source image is never overwritten.

Viewing and editing:
- The image is fitted to the window when opened. Zoom with the mouse wheel and
  pan with a middle-button drag.
- Drag with the left button to select a rectangle, then crop from the context
  menu.
- Rotate, flip, convert colors, and resize (percentage or pixels, optional
  aspect-ratio lock) from the context menu.
- Undo and redo with Ctrl+Z and Ctrl+Y.

Copy and paste:
- Ctrl+C copies the image or the selection. Ctrl+V pastes an image.
- A pasted image can be moved; commit it or retry from the context menu.

EXIF and saving:
- File and EXIF information is shown in a floating window; the EXIF text can be
  copied.
- File > Save as shows save options before the file picker. Quality (JPEG,
  WebP, and HEIC/HEIF, 0-100) and compression (PNG 0-9; TIFF 0 = none, 1-9 =
  LZW) can be set.
- If the file name has no extension, the extension of the selected file type
  is added. After saving, the saved file is loaded as the current image.
- Existing files and the source image are not overwritten.
- Dropping an image onto a displayed image asks for confirmation first.

Supported formats and codecs:
- Open: BMP, GIF, ICO, JPEG, JPEG XR, PNG, TIFF, Windows Media Photo, and DDS
  use Windows Imaging Component (WIC). WebP uses Qt's WebP image-format plugin.
  HEIC and HEIF need a compatible WIC codec installed on the computer.
- Save as: PNG, JPEG, BMP, TIFF, WebP, and HEIC/HEIF. TIFF is written through
  WIC. HEIC/HEIF saving needs a Windows HEIF encoder (HEIF Image Extensions and
  HEVC Video Extensions) and fails without it.

Command line:
- QuickImageView.exe --convert C:\path\to\source.png C:\path\to\output.bmp
  converts an image without showing a window. The exit code is 0 on success and
  2 on failure (the reason is written to crash.log). Existing files are never
  overwritten.

Language, theme, and Windows integration:
- The upper-right button shows a cyan globe and English or 日本語; click it to
  switch language.
- The menu order is File, Edit, Help. Help > Help opens the bundled help and
  Help > About opens the version dialog.
- File > Settings adds or removes QuickImageView in the Explorer image context
  menu (current user, HKCU).

Distribution files
------------------
- QuickImageView.exe and the Qt runtime (Qt6*.dll, platforms, imageformats,
  qml, and related files)
- C++ runtime DLLs (vcruntime140*.dll, msvcp140*.dll); no separate Visual C++
  Redistributable installation is needed
- readme.txt / readme_jp.txt
- history.txt / history_jp.txt
- LICENSE.txt / LICENSE_jp.txt
- third_party_licenses.md / third_party_licenses_jp.md
- libwebp-COPYING / libwebp-PATENTS
- Help is embedded in QuickImageView.exe in English and Japanese.

SHA-256
-------
GitHub Releases includes a CI-generated SHA256SUMS.txt for the ZIP:
https://github.com/maktak-105/QuickImageView/releases
Verify the ZIP with PowerShell:
Get-FileHash .\QuickImageView-binary.zip -Algorithm SHA256

License
-------
MIT License. See LICENSE.txt. QuickImageView uses Qt 6 as dynamic libraries;
see third_party_licenses.md. WebP support comes from Qt's WebP image-format
plugin, which contains libwebp. The libwebp BSD license and patent notice are
provided as libwebp-COPYING and libwebp-PATENTS.
