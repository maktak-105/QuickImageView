# Third-party licenses

[Japanese version third_party_licenses_jp.md](third_party_licenses_jp.md)

## Qt 6.10.3

QuickImageView uses Qt 6 (Core, Gui, Qml, Quick, Quick Controls 2, Quick Dialogs, Svg, and related modules). The Qt libraries are deployed as dynamic libraries (DLLs) next to the executable and are not modified by QuickImageView.

- Qt is available under the GNU Lesser General Public License version 3 and other licenses. See https://www.qt.io/licensing/ and https://doc.qt.io/qt-6/lgpl.html.

## libwebp (bundled in Qt's WebP image-format plugin)

WebP files are read and written by `imageformats/qwebp.dll`, the WebP image-format plugin of Qt's qtimageformats module, which contains Google's libwebp.

- License text: `libwebp-COPYING` in the distribution documents
- Patent notice: `libwebp-PATENTS` in the distribution documents
- License type: BSD-style three-clause license

Binary redistribution includes the libwebp copyright notice, license terms, and disclaimer in this document or the distribution documents. The names of Google or its contributors are not used to imply endorsement of the product.

The patent license terms in `PATENTS` also apply. When the Qt version is updated, review `libwebp-COPYING`, `libwebp-PATENTS`, and this document again.
