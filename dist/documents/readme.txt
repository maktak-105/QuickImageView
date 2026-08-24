QuickImageView - Distribution package v2.1.0

This document describes the package contents and current documented behavior. It
is not a UI-test result; untested requirements must not be treated as complete.

Requirements
------------
- Windows 10 / 11 (64-bit)
- Windows Imaging Component (included with Windows)

Usage
-----
Starting and opening images:
- Run QuickImageView.exe.
- Use File > Open image or Ctrl+O, or drag an image onto the window.
- Opening another image replaces the current image after confirmation.
- The source image is not overwritten.

Viewing and navigation:
- The image is fitted to the window when opened.
- Use zoom controls or the mouse wheel to zoom; drag to pan when zoomed.
- The information area and lower message area report image and operation status.

Selection and editing:
- Drag on the image to select a rectangular area.
- Edit > Crop crops to the selection.
- Edit > Rotate clockwise, Rotate counterclockwise, Flip horizontal, and Flip
  vertical perform image transformations.
- Edit > Undo and Edit > Redo, Ctrl+Z, and Ctrl+Y undo or redo changes.
- Resize accepts percentage and pixel dimensions. Aspect-ratio lock preserves
  the original ratio. Resize values are entered through the resize command.

Copy and paste:
- Edit > Copy or Ctrl+C copies the selection, or the full image when there is
  no selection. The context menu also provides Copy and Paste.
- Edit > Paste or Ctrl+V accepts internal QuickImageView copies and images from
  external Windows applications.
- Internal copy data is preferred. If unavailable, the Windows clipboard is
  read from CF_BITMAP, CF_DIB, or CF_DIBV5.
- A pasted image is movable until OK applies it. Retry cancels the operation;
  the applied paste can be undone.

EXIF and saving:
- Open image information from the information command or context menu.
- EXIF information and the lower message area use black backgrounds with white
  text. EXIF text can be copied from the EXIF window.
- Use File > Save as to choose destination, format, and file name.
- Existing destination files are not overwritten without confirmation.
- WebP output is included. HEIC/HEIF availability depends on installed codecs.

Supported formats and codecs:
- BMP, GIF, ICO, JPEG, JPEG XR, PNG, TIFF, Windows Media Photo, DDS,
  WebP, HEIC, and HEIF.
- BMP, GIF, ICO, JPEG, JPEG XR, PNG, TIFF, Windows Media Photo, and DDS use
  Windows Imaging Component (WIC) decoders.
- WebP input uses the Windows WIC WebP decoder and depends on that codec.
- WebP output uses statically linked libwebp 1.6.0 included in QuickImageView;
  no separate libwebp installation is required.
- HEIC and HEIF input/output depend on the Windows WIC codecs installed on the
  computer.

Language, theme, and Windows integration:
- The upper-right button shows a cyan globe and English or 日本語; click it to
  switch language.
- The menu order is File, Edit, Help. Help > Help opens the bundled help and Help > About opens the version dialog.
- The client area, buttons, menus, and menu popups use the dark theme.
- The PowerShell installer registers the context menu under the current user
  (HKCU). MSI context-menu and extension associations are independent optional
  features and associations are unselected by default.

Distribution files
------------------
- QuickImageView.exe
- readme.txt / readme_jp.txt
- history.txt / history_jp.txt
- LICENSE.txt / LICENSE_jp.txt
- Help is embedded in QuickImageView.exe in English and Japanese.
- libwebp-COPYING / libwebp-PATENTS

LICENSE_jp.txt is sourced from document/LICENSE_jp.txt and is included in the binary package and MSI.

License
-------
MIT License. See LICENSE.txt. WebP encoding in this build statically links
libwebp 1.6.0. Image loading and other WIC-backed formats use Windows Imaging
Component; HEIC/HEIF availability depends on the codecs installed on Windows.
The libwebp BSD license and patent notice are provided as libwebp-COPYING and
libwebp-PATENTS.
