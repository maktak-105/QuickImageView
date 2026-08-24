# QuickImageView Help
Version 3.1.2

## Open and view
Open a local image with **File > Open image**, `Ctrl+O`, or drag and drop. The image is displayed to fit the window.

## Rotate, flip, and edit
Use the **Edit** menu or the image area's context menu to rotate right, 180 degrees, or left by 90 degrees, flip horizontally or vertically, convert color, and resize. Drag with the left mouse button over the image to select a crop rectangle. Changes apply to the displayed image and never modify the original file.

## Copy and paste
Use `Ctrl+C` or **Edit > Copy image** to copy the displayed image to the Windows clipboard. Use `Ctrl+V` or **Edit > Paste image** to paste a clipboard image. Drag the pending paste with the left mouse button, then use the right-click **Commit paste** or **Retry paste** command.

## Save
**File > Save as** opens save options before the file picker. JPEG/WebP quality (0-100) and PNG/TIFF compression (0-9) can be set. Saving over the original or an existing output is rejected. HEIC/HEIF saving requires an available Windows WIC encoder.

## EXIF
**Help > EXIF information** displays WIC-retrieved make, model, and taken date in a floating modal window. Use **Copy EXIF** to copy the text to the clipboard. Images without metadata show `EXIF: none`.

## Undo and redo
Use `Ctrl+Z` or **Edit > Undo** to reverse the latest rotation or flip. Use `Ctrl+Y` or **Edit > Redo** to reapply an undone edit.

## Help and About
Use **Help > Help** to open this document. **Help > About** shows QuickImageView Ver. 3.1.2, its development environment, the author, and the creator badge. Both are dark modal views and close with **Close** or **OK**.

## Display language
Use the globe button in the upper-right labeled `English` / `日本語` to switch the menus, status messages, Help, and About dialog.

## Formats and codecs
BMP, GIF, ICO, JPEG, JPEG XR, PNG, TIFF, Windows Media Photo, and DDS are read with Windows Imaging Component (WIC). HEIC and HEIF can be read when a compatible Windows WIC codec is installed. WebP is read with libwebp 1.6.0 statically linked into QuickImageView, so no separate libwebp installation is required.

## Current migration scope
This Qt Quick build provides image loading, viewing, rotation, flipping, color conversion, crop, resize, movable paste, Undo/Redo, EXIF viewing/copying, Save as, Help, and About.
