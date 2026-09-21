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
- File > Save as shows save options before the file picker. Quality (JPEG and
  WebP, 0-100) and compression (PNG and TIFF, 0-9) can be set.
- Existing files and the source image are not overwritten.

Supported formats and codecs:
- Open: BMP, GIF, ICO, JPEG, JPEG XR, PNG, TIFF, Windows Media Photo, and DDS
  use Windows Imaging Component (WIC). WebP uses Qt's WebP image-format plugin.
  HEIC and HEIF need a compatible WIC codec installed on the computer.
- Save as: PNG, JPEG, BMP, and WebP. TIFF saving needs Qt's TIFF image plugin
  and HEIC/HEIF saving needs a WIC encoder; both fail when unavailable.

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
