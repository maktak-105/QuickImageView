# QuickImageView Specification

[日本語版 spec_jp.md](spec_jp.md)

QuickImageView is a Windows image viewer and editor implemented with Qt 6 (Qt Quick / QML), C++17, Windows Imaging Component (WIC), and CMake. Image processing is separated from the UI in GUI-independent C++. This document covers version 4.0.0.

## Viewing and operation

- Open an image from a command-line path, File > Open image, `Ctrl+O`, or drag and drop. When an image is already displayed, a dropped image is opened only after the user confirms.
- Fit the image to the window, zoom with the mouse wheel, and pan with a middle-button drag.
- Select a rectangle with a left-button drag and crop it from the context menu.
- Show the file name, dimensions, format, and file size, and representative EXIF fields (make, model, capture date) when present. EXIF is shown in a floating window that can be copied as text; editing EXIF or metadata is out of scope.

## Editing

- Rotate right 90 degrees, 180 degrees, or left 90 degrees; flip horizontally or vertically.
- Convert to full color, 256 colors, or grayscale.
- Resize by a user-specified positive percentage or pixel size, with an aspect-ratio lock option.
- Undo and redo with `Ctrl+Z` and `Ctrl+Y`.
- Copy the image or the selection with `Ctrl+C`. Paste an image with `Ctrl+V`; the pasted image keeps its original scale, can be moved within the destination image, and is committed or retried from the context menu.

## Saving

- File > Save as shows save options first, then the file picker.
- Quality (JPEG, WebP, and HEIC/HEIF, 0-100) and compression (PNG 0-9; TIFF 0 = uncompressed, 1-9 = LZW) are user-specified values, not a fixed list.
- The source image and existing output files are never overwritten.
- Save formats: PNG, JPEG, BMP, TIFF, WebP, HEIC/HEIF. TIFF is written through WIC (no Qt plugin needed). HEIC/HEIF requires a Windows HEIF encoder (HEIF Image Extensions and HEVC Video Extensions); saving fails when it is missing.
- When the file name has no extension, the extension of the selected file type is added.
- After a successful save, the saved file is reloaded without a confirmation and shown as the current image.

## Windows integration

- File > Settings registers or removes the QuickImageView entry in the Explorer image context menu (current user, HKCU).
- `scripts/install.ps1` installs per user under `%LOCALAPPDATA%` and registers the entry unless `-NoRegisterContextMenu` is given.

## Command line

- `QuickImageView.exe --convert <source> <destination>` converts an image without showing a window. The format follows the destination extension. The exit code is 0 on success and 2 on failure (the reason is written to `crash.log`). The source and existing files are never overwritten.

## UI

- Dark theme (including the title bar), Japanese/English switch (menus, dialogs, information, EXIF, help), in-app Help, and an About dialog that shows the version, development environment, author, and creator badge.

## Supported image formats

- Open: BMP, GIF, ICO, JPEG, JPEG XR, PNG, TIFF, Windows Media Photo, and DDS through WIC; WebP through Qt's WebP image-format plugin; HEIC and HEIF through an installed WIC codec.
- The Open dialog lists JPG/JPEG, PNG, TIFF, BMP, GIF, WebP, HEIC, and HEIF.

## Out of scope

- Explorer thumbnail shell extensions
- Previous/next navigation through a folder, slide shows, and printing
- EXIF or metadata editing
- Saving directly over the source image

## Tests

CTest runs `qiv_controller_tests` (Qt Test, GUI-independent controller and engine) and `qiv_qml_tests` (Qt Quick Test, offscreen).
