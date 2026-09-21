# QuickImageView Help
Version 4.1.0

## Open and view
Open a local image with **File > Open image**, `Ctrl+O`, or drag and drop. The image is displayed to fit the window. When an image is already displayed, dropping another image asks for confirmation before it is opened.

## Rotate, flip, and edit
Use the image area's context menu (right-click) to rotate right, 180 degrees, or left by 90 degrees, flip horizontally or vertically, convert color, and resize. Drag with the left mouse button over the image to select a crop rectangle. Changes apply to the displayed image and never modify the original file.

## Copy and paste
Use `Ctrl+C` or the right-click **Copy image** command to copy the displayed image to the Windows clipboard. Use `Ctrl+V` or the right-click **Paste image** command to paste a clipboard image. Drag the pending paste with the left mouse button, then use the right-click **Commit paste** or **Retry paste** command.

## Save
**File > Save as** opens save options before the file picker. JPEG/WebP/HEIC quality (0-100) and PNG/TIFF compression (0-9; for TIFF, 0 = uncompressed and 1-9 = LZW) can be set. If the file name has no extension, the extension of the selected file type is added. Saving over the original or an existing output is rejected. HEIC/HEIF saving requires a Windows HEIF encoder (HEIF Image Extensions and HEVC Video Extensions). After a successful save, the saved file is loaded as the current image.

## EXIF
**Help > EXIF information** displays WIC-retrieved make, model, and taken date in a floating modal window. Use **Copy EXIF** to copy the text to the clipboard. Images without metadata show `EXIF: none`.

## Undo and redo
Use `Ctrl+Z` or **Edit > Undo** to reverse the latest rotation or flip. Use `Ctrl+Y` or **Edit > Redo** to reapply an undone edit.

## Help and About
Use **Help > Help** to open this document. **Help > About** shows QuickImageView Ver. 4.1.0, its development environment, the author, and the creator badge. Both are dark modal views and close with **Close** or **OK**.

## Display language
Use the globe button in the upper-right labeled `English` / `日本語` to switch the menus, status messages, Help, and About dialog.

## Settings
**File > Settings** adds QuickImageView to the Explorer image context menu and sets the window size. Enter the width and height of the window's content area in pixels (480 x 320 or larger; the default is 720 x 480). The size is used the next time QuickImageView starts and is applied to the current window unless it is maximized.

## Command line
Run `QuickImageView.exe --convert <source> <destination>` to convert an image without opening a window. The format follows the destination extension. The exit code is 0 on success and 2 on failure; the reason is written to `crash.log`. Existing files are never overwritten.

## Formats and codecs
BMP, GIF, ICO, JPEG, JPEG XR, PNG, TIFF, Windows Media Photo, and DDS are read with Windows Imaging Component (WIC). HEIC and HEIF can be read when a compatible Windows WIC codec is installed. WebP is read with Qt's WebP image-format plugin, so no Windows WebP codec is required.

## Features
QuickImageView provides image loading, viewing, rotation, flipping, color conversion, crop, resize, movable paste, Undo/Redo, EXIF viewing/copying, Save as, Help, and About.
