# QuickImageView Specification

[日本語版 spec_jp.md](spec_jp.md)

QuickImageView is a Windows image viewer and editor implemented with C++17, Win32, Windows Imaging Component, and CMake. This document covers version 3.1.3.

## Preserved existing behavior

- Display the file name, dimensions, format, and file size.
- Display representative EXIF fields when present (make, model, capture date). Editing EXIF or metadata is out of scope.
- New features and UI changes must not remove these displays.

## Features

- Open an image from a command line path or the File > Open dialog.
- Fit, zoom, and pan an image.
- Convert to a new file without overwriting the original.
- Resize using user-specified positive percentage or pixel dimensions, crop, rotate, mirror, and convert color modes; a fixed preset list is not the requirement.
- Undo/redo edits and copy/paste images through the Windows clipboard.
- Apply user-specified JPEG quality and PNG compression; WebP output uses bundled libwebp and WebP input uses the Windows WIC decoder. HEIC/HEIF use installed WIC codecs. Quality is not restricted to a fixed candidate list.
- Install and optionally register an Explorer context-menu entry.
- Provide independent MSI features for the context menu and each supported extension, disabled by default.
- Include the executable, bilingual help, distribution documents, MIT License, and libwebp notices in the MSI.

## Safety rules

- The source image is never changed, deleted, or overwritten.
- A pasted image keeps its original scale and is constrained to the destination image.
- A successful conversion reloads the destination without a confirmation prompt.
- EXIF is shown in a separate floating window and can be copied as Unicode text.
- Information and status bars use a black background and white text.
- The application uses Windows Segoe UI without installing a private font.
- The Japanese/English switch updates menus, dialogs, information bars, EXIF, and help.
- Help > About opens a dark modal card showing version 3.1.3, development environment, author, and the official creator badge.

## Supported image formats

QuickImageView opens BMP, GIF, ICO, JPEG, JPEG XR, PNG, TIFF, Windows Media Photo, DDS, WebP, HEIC and HEIF when the corresponding decoder is present. Windows includes WIC decoders for BMP, GIF, ICO, JPEG, JPEG XR, PNG, TIFF, Windows Media Photo and DDS. WebP input uses the Windows WIC WebP decoder, WebP output uses bundled libwebp 1.6.0, and HEIC/HEIF depend on installed WIC codecs. The current Open dialog lists JPG/JPEG, PNG, TIFF, BMP, GIF, WebP, HEIC and HEIF.

- The original image is never overwritten or deleted.
- Existing output files are rejected.
- UI and Explorer behavior is tested by `tests/python/ui_test.py` using pywinauto/uiautomation against the installed application.
