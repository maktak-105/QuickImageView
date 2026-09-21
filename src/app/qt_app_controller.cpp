#include "qt_app_controller.h"

#include "image_engine.h"
#include "quick_image_provider.h"

#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImageReader>
#include <QPainter>
#include <QRect>
#include <QStringList>
#include <QTransform>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>

QtAppController::QtAppController(QuickImageProvider* imageProvider, QObject* parent)
    : QObject(parent), statusText_(localize(QStringLiteral("画像を開いてください。"), QStringLiteral("Open an image to begin."))),
      imageProvider_(imageProvider) {}

QString QtAppController::appName() const {
    return QStringLiteral("QuickImageView");
}

QString QtAppController::appVersion() const {
    return QStringLiteral(QUICKIMAGEVIEW_VERSION);
}

bool QtAppController::english() const {
    return english_;
}

QUrl QtAppController::imageSource() const {
    return imageSource_;
}

bool QtAppController::hasImage() const {
    return imageSource_.isValid() && !imageSource_.isEmpty();
}

bool QtAppController::canUndo() const {
    return !undoHistory_.isEmpty();
}

bool QtAppController::canRedo() const {
    return !redoHistory_.isEmpty();
}

QString QtAppController::imageName() const {
    return imageName_;
}

QString QtAppController::statusText() const {
    return statusText_;
}

QString QtAppController::helpText() const {
    return readEmbeddedHelp(english_ ? QStringLiteral(":/src/app/help/help.md")
                                   : QStringLiteral(":/src/app/help/help_jp.md"));
}

QString QtAppController::exifText() const {
    if (!hasImage()) return {};
    return ImageEngine::formatExifText(exifFields_, english_);
}

namespace {
QString formatByteSize(qint64 bytes) {
    if (bytes < 1024) return QStringLiteral("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
}
}

QString QtAppController::fileInfoText() const {
    if (!hasImage()) {
        return localize(QStringLiteral("画像を開いてください。"), QStringLiteral("Open an image to begin."));
    }
    QStringList lines;
    lines << localize(QStringLiteral("ファイル名: "), QStringLiteral("File: "))
             + (imageName_.isEmpty() ? localize(QStringLiteral("(未保存)"), QStringLiteral("(unsaved)")) : imageName_);
    lines << localize(QStringLiteral("画像サイズ: "), QStringLiteral("Dimensions: "))
             + QStringLiteral("%1 x %2").arg(image_.width()).arg(image_.height());
    if (!sourcePath_.isEmpty()) {
        const QFileInfo info(sourcePath_);
        if (info.exists()) {
            lines << localize(QStringLiteral("ファイルサイズ: "), QStringLiteral("File size: "))
                     + formatByteSize(info.size());
            lines << localize(QStringLiteral("形式: "), QStringLiteral("Format: ")) + info.suffix().toUpper();
            lines << localize(QStringLiteral("場所: "), QStringLiteral("Location: "))
                     + QDir::toNativeSeparators(info.absolutePath());
        }
    }
    return lines.join(QLatin1Char('\n'));
}

int QtAppController::imageWidth() const { return image_.width(); }
int QtAppController::imageHeight() const { return image_.height(); }
bool QtAppController::pastePending() const { return !pasteImage_.isNull(); }
QUrl QtAppController::pasteSource() const { return pasteSource_; }
int QtAppController::pasteX() const { return pasteX_; }
int QtAppController::pasteY() const { return pasteY_; }
int QtAppController::pasteWidth() const { return pasteImage_.width(); }
int QtAppController::pasteHeight() const { return pasteImage_.height(); }

void QtAppController::openImage(const QUrl& fileUrl) {
    if (!fileUrl.isLocalFile()) {
        statusText_ = localize(QStringLiteral("ローカル画像ファイルを指定してください。"),
                               QStringLiteral("Select a local image file."));
        emit statusChanged();
        return;
    }

    const QFileInfo info(fileUrl.toLocalFile());
    if (!info.isFile()) {
        statusText_ = localize(QStringLiteral("画像ファイルが見つかりません。"),
                               QStringLiteral("The image file was not found."));
        emit statusChanged();
        return;
    }

    QString error;
    const QImage image = ImageEngine::load(info.absoluteFilePath(), &error);
    if (image.isNull()) {
        statusText_ = localize(QStringLiteral("画像を読み込めません: "), QStringLiteral("Could not load image: ")) + error;
        emit statusChanged();
        return;
    }

    clearEditHistory();
    clearPasteState();
    image_ = image;
    if (imageProvider_) imageProvider_->setImage(image_);
    imageSource_ = QUrl(QStringLiteral("image://quickimage/current/%1").arg(++imageRevision_));
    imageName_ = info.fileName();
    sourcePath_ = info.absoluteFilePath();
    exifFields_ = ImageEngine::readExif(sourcePath_);
    statusText_ = localize(QStringLiteral("画像を読み込みました: "), QStringLiteral("Image loaded: ")) + imageName_;
    emit imageChanged();
    emit imageOpened();
    emit exifChanged();
    emit fileInfoChanged();
    emit statusChanged();
}

void QtAppController::copyExif() {
    const QString text = exifText();
    if (text.isEmpty()) return;
    QGuiApplication::clipboard()->setText(text);
    statusText_ = localize(QStringLiteral("EXIF情報をコピーしました。"), QStringLiteral("EXIF information copied."));
    emit statusChanged();
}

void QtAppController::saveImage(const QUrl& fileUrl) {
    if (!hasImage() || !fileUrl.isLocalFile()) return;
    const QString outputPath = QFileInfo(fileUrl.toLocalFile()).absoluteFilePath();
    if (!sourcePath_.isEmpty() && QFileInfo(outputPath).absoluteFilePath() == QFileInfo(sourcePath_).absoluteFilePath()) {
        statusText_ = localize(QStringLiteral("原本と同じ場所には保存できません。原本は変更していません。"),
                               QStringLiteral("The original image cannot be overwritten."));
        emit statusChanged();
        return;
    }
    if (QFileInfo::exists(outputPath)) {
        statusText_ = localize(QStringLiteral("既存ファイルへの上書きは禁止されています。"),
                               QStringLiteral("Overwriting an existing file is not allowed."));
        emit statusChanged();
        return;
    }
    QString error;
    if (!ImageEngine::save(image_, outputPath, saveOptions_, &error)) {
        statusText_ = localize(QStringLiteral("画像を保存できません: "), QStringLiteral("Could not save image: ")) + error;
        emit statusChanged();
        return;
    }
    statusText_ = localize(QStringLiteral("別ファイルとして保存しました。"), QStringLiteral("Saved as a separate file."));
    emit statusChanged();
}

void QtAppController::clearImage() {
    if (!hasImage()) return;
    imageSource_ = QUrl{};
    image_ = {};
    if (imageProvider_) imageProvider_->clear();
    clearEditHistory();
    clearPasteState();
    imageName_.clear();
    sourcePath_.clear();
    exifFields_ = {};
    statusText_ = localize(QStringLiteral("画像を閉じました。"), QStringLiteral("Image closed."));
    emit imageChanged();
    emit imageOpened();
    emit exifChanged();
    emit fileInfoChanged();
    emit statusChanged();
}

void QtAppController::rotateRight() {
    if (!hasImage()) return;
    applyImageEdit(image_.transformed(QTransform().rotate(90)),
                         QStringLiteral("右へ90度回転しました。"), QStringLiteral("Rotated right 90 degrees."));
}

void QtAppController::rotate180() {
    if (!hasImage()) return;
    applyImageEdit(image_.transformed(QTransform().rotate(180)),
                   QStringLiteral("180度回転しました。"), QStringLiteral("Rotated 180 degrees."));
}

void QtAppController::rotateLeft() {
    if (!hasImage()) return;
    applyImageEdit(image_.transformed(QTransform().rotate(-90)),
                         QStringLiteral("左へ90度回転しました。"), QStringLiteral("Rotated left 90 degrees."));
}

void QtAppController::resizeImage(int width, int height) {
    if (!hasImage()) return;
    const QImage resized = ImageEngine::resize(image_, width, height);
    if (resized.isNull()) {
        statusText_ = localize(QStringLiteral("指定したサイズが不正です。"), QStringLiteral("The requested size is invalid."));
        emit statusChanged();
        return;
    }
    applyImageEdit(resized, QStringLiteral("リサイズしました。"), QStringLiteral("Resized image."));
}

void QtAppController::cropImage(int x, int y, int width, int height) {
    if (!hasImage() || x < 0 || y < 0 || width <= 0 || height <= 0 ||
        x + width > image_.width() || y + height > image_.height()) {
        statusText_ = localize(QStringLiteral("切り抜き範囲が不正です。"), QStringLiteral("The crop rectangle is invalid."));
        emit statusChanged();
        return;
    }
    applyImageEdit(image_.copy(x, y, width, height), QStringLiteral("選択範囲を切り抜きました。"),
                   QStringLiteral("Cropped the selected area."));
}

void QtAppController::convertToFullColor() {
    if (!hasImage()) return;
    applyImageEdit(ImageEngine::convertColor(image_, 0), QStringLiteral("フルカラーへ変換しました。"),
                   QStringLiteral("Converted to full color."));
}

void QtAppController::convertTo256Colors() {
    if (!hasImage()) return;
    applyImageEdit(ImageEngine::convertColor(image_, 1), QStringLiteral("256色へ変換しました。"),
                   QStringLiteral("Converted to 256 colors."));
}

void QtAppController::convertToGrayscale() {
    if (!hasImage()) return;
    applyImageEdit(ImageEngine::convertColor(image_, 2), QStringLiteral("グレースケールへ変換しました。"),
                   QStringLiteral("Converted to grayscale."));
}

void QtAppController::flipHorizontal() {
    if (!hasImage()) return;
    applyImageEdit(image_.flipped(Qt::Horizontal), QStringLiteral("左右反転しました。"),
                         QStringLiteral("Flipped horizontally."));
}

void QtAppController::flipVertical() {
    if (!hasImage()) return;
    applyImageEdit(image_.flipped(Qt::Vertical), QStringLiteral("上下反転しました。"),
                         QStringLiteral("Flipped vertically."));
}

void QtAppController::copyImage() {
    if (!hasImage()) return;
    QGuiApplication::clipboard()->setImage(image_);
    statusText_ = localize(QStringLiteral("画像をクリップボードへコピーしました。"),
                           QStringLiteral("Image copied to the clipboard."));
    emit statusChanged();
}

void QtAppController::copyImageRegion(int x, int y, int width, int height) {
    if (!hasImage()) return;
    const QRect region = QRect(x, y, width, height).intersected(image_.rect());
    if (region.isEmpty()) {
        copyImage();
        return;
    }
    QGuiApplication::clipboard()->setImage(image_.copy(region));
    statusText_ = localize(QStringLiteral("選択範囲をクリップボードへコピーしました。"),
                           QStringLiteral("The selected area was copied to the clipboard."));
    emit statusChanged();
}

void QtAppController::resizeImageBy(int width, int height, bool percent, bool keepAspectRatio) {
    if (!hasImage()) return;
    if (width <= 0 || height <= 0 || width > 100000 || height > 100000) {
        statusText_ = localize(QStringLiteral("指定したサイズが不正です。"), QStringLiteral("The requested size is invalid."));
        emit statusChanged();
        return;
    }
    // 正本のWin32版と同じ順序で、縦横比の補正を先に行ってから百分率を画素数へ換算する。
    if (keepAspectRatio) {
        if (percent) height = width;
        else if (image_.width() > 0) {
            height = qMax(1, qRound(static_cast<double>(width) * image_.height() / image_.width()));
        }
    }
    if (percent) {
        width = qMax(1, static_cast<int>(image_.width() * (width / 100.0)));
        height = qMax(1, static_cast<int>(image_.height() * (height / 100.0)));
    }
    resizeImage(width, height);
}

void QtAppController::pasteImage() {
    const QImage clipboardImage = QGuiApplication::clipboard()->image();
    if (clipboardImage.isNull()) {
        statusText_ = localize(QStringLiteral("クリップボードに画像がありません。"),
                               QStringLiteral("The clipboard does not contain an image."));
        emit statusChanged();
        return;
    }
    if (!hasImage()) {
        imageName_ = localize(QStringLiteral("クリップボード画像"), QStringLiteral("Clipboard image"));
        clearEditHistory();
        updateDisplayedImage(clipboardImage, QStringLiteral("クリップボードから画像を貼り付けました。"),
                             QStringLiteral("Image pasted from the clipboard."));
        return;
    }
    pasteImage_ = clipboardImage.convertToFormat(QImage::Format_ARGB32);
    pasteX_ = qMax(0, (image_.width() - pasteImage_.width()) / 2);
    pasteY_ = qMax(0, (image_.height() - pasteImage_.height()) / 2);
    pasteX_ = qMin(pasteX_, qMax(0, image_.width() - pasteImage_.width()));
    pasteY_ = qMin(pasteY_, qMax(0, image_.height() - pasteImage_.height()));
    if (imageProvider_) imageProvider_->setPasteImage(pasteImage_);
    pasteSource_ = QUrl(QStringLiteral("image://quickimage/paste/%1").arg(++imageRevision_));
    statusText_ = localize(QStringLiteral("貼り付け画像を移動できます。右クリックで確定またはやり直してください。"),
                           QStringLiteral("Move the pasted image. Right-click to commit or retry."));
    emit pasteChanged();
    emit statusChanged();
}

void QtAppController::movePaste(int x, int y) {
    if (!pastePending()) return;
    const int boundedX = qBound(0, x, qMax(0, image_.width() - pasteImage_.width()));
    const int boundedY = qBound(0, y, qMax(0, image_.height() - pasteImage_.height()));
    // 位置が変わらないドラッグで通知を出すと、QML側の再評価が無駄に走って追従が鈍る。
    if (boundedX == pasteX_ && boundedY == pasteY_) return;
    pasteX_ = boundedX;
    pasteY_ = boundedY;
    emit pasteChanged();
}

void QtAppController::commitPaste() {
    if (!pastePending()) return;
    QImage composed = image_.copy();
    QPainter painter(&composed);
    painter.drawImage(pasteX_, pasteY_, pasteImage_);
    painter.end();
    clearPasteState();
    applyImageEdit(composed, QStringLiteral("貼り付け画像を確定しました。"),
                   QStringLiteral("Pasted image committed."));
}

void QtAppController::retryPaste() {
    if (!pastePending()) return;
    statusText_ = localize(QStringLiteral("貼り付け画像の移動を続けます。"),
                           QStringLiteral("Paste movement continues."));
    emit statusChanged();
}

void QtAppController::setSaveOptions(int quality, int compression) {
    saveOptions_.quality = qBound(0, quality, 100);
    saveOptions_.compression = qBound(0, compression, 9);
}

void QtAppController::undo() {
    if (!canUndo()) return;
    redoHistory_.append(image_);
    updateDisplayedImage(undoHistory_.takeLast(), QStringLiteral("元に戻しました。"), QStringLiteral("Undo completed."));
    emit editHistoryChanged();
}

void QtAppController::redo() {
    if (!canRedo()) return;
    undoHistory_.append(image_);
    updateDisplayedImage(redoHistory_.takeLast(), QStringLiteral("やり直しました。"), QStringLiteral("Redo completed."));
    emit editHistoryChanged();
}

void QtAppController::setEnglish(bool enabled) {
    if (english_ == enabled) return;
    english_ = enabled;
    statusText_ = hasImage()
        ? localize(QStringLiteral("画像を表示しています: "), QStringLiteral("Viewing image: ")) + imageName_
        : localize(QStringLiteral("画像を開いてください。"), QStringLiteral("Open an image to begin."));
    emit languageChanged();
    emit fileInfoChanged();
    emit exifChanged();
    emit statusChanged();
}

QString QtAppController::localize(const QString& japanese, const QString& englishText) const {
    return english_ ? englishText : japanese;
}

QString QtAppController::readEmbeddedHelp(const QString& resourcePath) const {
    QFile helpFile(resourcePath);
    if (!helpFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return localize(QStringLiteral("ヘルプを読み込めません。"), QStringLiteral("Help could not be loaded."));
    }
    return QString::fromUtf8(helpFile.readAll());
}

void QtAppController::updateDisplayedImage(const QImage& image, const QString& noticeJapanese, const QString& noticeEnglish) {
    image_ = image;
    if (imageProvider_) imageProvider_->setImage(image_);
    imageSource_ = QUrl(QStringLiteral("image://quickimage/current/%1").arg(++imageRevision_));
    statusText_ = localize(noticeJapanese, noticeEnglish);
    emit imageChanged();
    emit fileInfoChanged();
    emit statusChanged();
}

void QtAppController::clearPasteState() {
    if (pasteImage_.isNull() && pasteSource_.isEmpty()) return;
    pasteImage_ = {};
    pasteSource_ = QUrl{};
    pasteX_ = 0;
    pasteY_ = 0;
    if (imageProvider_) imageProvider_->clearPaste();
    emit pasteChanged();
}

void QtAppController::applyImageEdit(const QImage& image, const QString& noticeJapanese, const QString& noticeEnglish) {
    if (image.isNull()) return;
    undoHistory_.append(image_);
    redoHistory_.clear();
    updateDisplayedImage(image, noticeJapanese, noticeEnglish);
    emit editHistoryChanged();
}

void QtAppController::clearEditHistory() {
    const bool changed = !undoHistory_.isEmpty() || !redoHistory_.isEmpty();
    undoHistory_.clear();
    redoHistory_.clear();
    if (changed) emit editHistoryChanged();
}

bool QtAppController::isContextMenuRegistered() const {
    HKEY key = nullptr;
    const wchar_t* subKey = L"Software\\Classes\\SystemFileAssociations\\image\\shell\\QuickImageView\\command";
    if (RegOpenKeyExW(HKEY_CURRENT_USER, subKey, 0, KEY_READ, &key) == ERROR_SUCCESS) {
        RegCloseKey(key);
        return true;
    }
    return false;
}

bool QtAppController::setContextMenuRegistered(bool enable) {
    const wchar_t* rootSubKey = L"Software\\Classes\\SystemFileAssociations\\image\\shell\\QuickImageView";
    if (!enable) {
        LSTATUS status = RegDeleteTreeW(HKEY_CURRENT_USER, rootSubKey);
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
        emit contextMenuRegisteredChanged();
        return (status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND);
    }

    const QString appExePath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString icoCandidate = QDir::toNativeSeparators(appDir + QStringLiteral("/QuickImageView.ico"));
    const QString iconPath = QFile::exists(icoCandidate) ? icoCandidate : appExePath;

    HKEY hKey = nullptr;
    LSTATUS status = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        rootSubKey,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE | KEY_CREATE_SUB_KEY,
        nullptr,
        &hKey,
        nullptr
    );
    if (status != ERROR_SUCCESS) {
        return false;
    }

    const QString menuText = english_ ? QStringLiteral("Open with QuickImageView") : QStringLiteral("QuickImageViewで開く");
    const std::wstring wMenuText = menuText.toStdWString();
    RegSetValueExW(
        hKey,
        nullptr,
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(wMenuText.c_str()),
        static_cast<DWORD>((wMenuText.length() + 1) * sizeof(wchar_t))
    );

    const std::wstring wIconPath = iconPath.toStdWString();
    RegSetValueExW(
        hKey,
        L"Icon",
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(wIconPath.c_str()),
        static_cast<DWORD>((wIconPath.length() + 1) * sizeof(wchar_t))
    );

    HKEY hCmdKey = nullptr;
    status = RegCreateKeyExW(
        hKey,
        L"command",
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        nullptr,
        &hCmdKey,
        nullptr
    );
    if (status == ERROR_SUCCESS) {
        const QString commandValue = QStringLiteral("\"%1\" \"%2\"").arg(appExePath, QStringLiteral("%1"));
        const std::wstring wCmdValue = commandValue.toStdWString();
        RegSetValueExW(
            hCmdKey,
            nullptr,
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(wCmdValue.c_str()),
            static_cast<DWORD>((wCmdValue.length() + 1) * sizeof(wchar_t))
        );
        RegCloseKey(hCmdKey);
    }
    RegCloseKey(hKey);

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    emit contextMenuRegisteredChanged();
    return true;
}
