# QuickImageView Specification

[日本語版 spec_jp.md](spec_jp.md)

QuickImageView is a Windows image viewer implemented with C++17, Win32, and Windows Imaging Component.

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
- Apply user-specified JPEG quality and PNG compression; WebP and HEIC/HEIF use installed WIC codecs. Quality is not restricted to a fixed candidate list.
- Install and optionally register an Explorer context-menu entry.

## Safety rules

- The original image is never overwritten or deleted.
- Existing output files are rejected.
- UI and Explorer behavior is tested by sending operations to a running window from `tests/verify_ui.ps1`.

## Verification

Run the single completion command:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tests\verify_goal.ps1
```

It validates the goal and invariant contracts, performs a clean build, runs CTest, executes data-backed edit self-tests, drives the build UI, installs to a temporary directory, compares SHA-256 hashes, and drives the installed UI. Any failed gate means the work is incomplete.
