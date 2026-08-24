#pragma once

#include <QImage>
#include <QMutex>
#include <QQuickImageProvider>

class QuickImageProvider final : public QQuickImageProvider {
public:
    QuickImageProvider();

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;
    void setImage(const QImage& image);
    void setPasteImage(const QImage& image);
    void clear();
    void clearPaste();

private:
    QMutex mutex_;
    QImage image_;
    QImage pasteImage_;
};
