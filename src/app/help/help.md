# QuickImageView Help

Version 3.1.3

## Open and view

Open an image with **File > Open image**, `Ctrl+O`, a command-line path, or drag and drop. The viewer fits the image to the window; use the mouse wheel to zoom and the middle mouse button to pan.

## EXIF information

Open **Help > EXIF information** to show the EXIF window beside the image. It uses a black background with white text, displays make, model, and capture date when available, and reports when no EXIF is present. Use **Copy EXIF** to copy the displayed text to the clipboard.

## Select, copy, and paste

Drag with the left mouse button to select an area. Its pixel size is shown at the upper-left of the selection. `Ctrl+C` copies the selected area, or the full image when no selection is active.

Paste with `Ctrl+V` or **Edit > Paste image**. The pasted image remains a movable overlay at its original image scale. Drag to move it inside the destination image. Right-click and choose **OK** to merge it, or **Retry** to continue moving it.

## Save safely

Choose **Save as another format** from the right-click menu. QuickImageView never overwrites the source image or an existing destination file. After a successful save, it loads the new file automatically.

## Language and Windows integration

Use the upper-right globe button labeled `English` / `日本語` to switch menus, dialogs, information bars, EXIF, and help. The MSI offers optional Explorer context-menu registration and optional associations for JPEG, PNG, TIFF, BMP/GIF, WebP, and HEIC/HEIF. Leave these options unchecked if you do not want to change Windows integration.

## About and version information

Open **Help > About** to view QuickImageView version 3.1.3, the development environment, author information, and the creator badge. The dialog is a dark modal card and closes with **OK** or the close button.

## Formats and codecs

QuickImageView supports BMP, GIF, ICO, JPEG, JPEG XR, PNG, TIFF, Windows Media Photo, DDS, WebP, HEIC, and HEIF when the corresponding decoder is available.

Windows Imaging Component (WIC) provides the standard decoders for BMP, GIF, ICO, JPEG, JPEG XR, PNG, TIFF, Windows Media Photo, and DDS. WebP input uses the Windows WIC WebP decoder, so opening WebP depends on that codec being available. WebP output uses the statically linked libwebp 1.6.0 included in QuickImageView and does not require a separate libwebp installation. HEIC and HEIF input/output depend on the Windows WIC codecs installed on the computer.
