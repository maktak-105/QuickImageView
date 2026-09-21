# QuickImageView Changelog

## v4.1.0 — 2026-09-21

- Fixed a hang when the window was moved between monitors with different display scaling (present in v4.0.0).
- The default window size is now 720 x 480 (minimum 480 x 320), and File > Settings sets the window size in pixels. The value is saved in `%LOCALAPPDATA%\maktak-105\QuickImageView\settings.ini`, used at the next start, and applied to the current window.
- Saving supports PNG, JPEG, BMP, TIFF, WebP, and HEIC/HEIF. TIFF is written through WIC (no Qt plugin needed), and HEIC/HEIF saving needs a Windows HEIF encoder.
- Restored from 3.x in the Qt version: `--convert` command-line conversion, the dark title bar, the confirmation before a dropped image replaces the displayed one, reloading the saved file after Save as, adding a missing extension from the selected file type, and the HEIC/HEIF quality option.
- The distribution contains only what the application uses: native Windows file dialogs instead of QtQuick.Dialogs, no Svg, TIFF, GIF or ICO image plugins, no QML tooling metadata, and the MinGW runtime. The release is built with MinGW, like the local build.
- Corrected the help text that mentioned Edit-menu items the Qt UI does not have.

## v4.0.0 — 2026-09-21 (withdrawn)

Withdrawn because it hung when the window was moved between monitors with different display scaling. Use v4.1.0.

- Replaced the Win32 implementation with the Qt 6 (Qt Quick / QML) implementation as the main line.
- Releases are an unsigned ZIP (`QuickImageView-binary.zip`) with a CI-generated `SHA256SUMS.txt`.
- Rebuilt the UI in Qt Quick and added a Settings dialog for the Explorer context-menu entry.
- Removed the MSI installer and the Win32 self-test and UI test scripts.
- WebP is now handled by Qt's WebP image-format plugin instead of a statically linked libwebp.
- The Win32 implementation remains available at the v3.x release tags (up to v3.1.3).

## v3.1.3 — 2026-09-20

- Fixed the project restructure, build/package paths, and release verification.
- Added the WiX MSI installer to GitHub Releases and included its SHA-256 in the checksum list.

## Versioning rules

- First digit: new features
- Second digit: bug fixes
- Third digit: documentation and other changes

## 2.1.0 (2026-08-24)

- Improved help readability with indented lists, spacing, and an explicit version.
- Removed non-functional help source-link text from the in-app help.
- Updated the application, About dialog, executable metadata, and installer to 2.1.0.

## 2.0.0 (2026-08-24)

- Updated the UI to the Quick-series dark layout and added Help > About.
- Embedded the bilingual help and the official creator badge in the executable.
- Synchronized version metadata, distribution documents, and MSI output naming.

## 1.0.0 (2026-08-23)

- Added selection-only clipboard copy, overlay paste, and destination reload after Save As.
- Added dark information bars, Segoe UI, an application icon, and Japanese/English UI switching.
- Added MSI authoring with optional Explorer context menu and per-extension file associations.
