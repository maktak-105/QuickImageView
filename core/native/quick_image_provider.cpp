#include "quick_image_provider.h"

#include <QMutexLocker>

QuickImageProvider::QuickImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image) {}

QImage QuickImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize) {
    QMutexLocker lock(&mutex_);
    QImage result = id.startsWith(QStringLiteral("paste")) ? pasteImage_ : image_;
    if (size) *size = result.size();
    if (requestedSize.isValid()) {
        result = result.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return result;
}

void QuickImageProvider::setImage(const QImage& image) {
    QMutexLocker lock(&mutex_);
    image_ = image;
}

void QuickImageProvider::setPasteImage(const QImage& image) {
    QMutexLocker lock(&mutex_);
    pasteImage_ = image;
}

void QuickImageProvider::clear() {
    QMutexLocker lock(&mutex_);
    image_ = {};
}

void QuickImageProvider::clearPaste() {
    QMutexLocker lock(&mutex_);
    pasteImage_ = {};
}
