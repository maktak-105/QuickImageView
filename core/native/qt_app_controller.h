#pragma once

#include <QObject>
#include <QImage>
#include <QUrl>
#include <QVector>

#include "image_engine.h"

class QuickImageProvider;

class QtAppController final : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString appName READ appName CONSTANT)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(bool english READ english NOTIFY languageChanged)
    Q_PROPERTY(QUrl imageSource READ imageSource NOTIFY imageChanged)
    Q_PROPERTY(bool hasImage READ hasImage NOTIFY imageChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY editHistoryChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY editHistoryChanged)
    Q_PROPERTY(QString imageName READ imageName NOTIFY imageChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QString helpText READ helpText NOTIFY languageChanged)
    Q_PROPERTY(QString exifText READ exifText NOTIFY exifChanged)
    Q_PROPERTY(QString fileInfoText READ fileInfoText NOTIFY fileInfoChanged)
    Q_PROPERTY(int imageWidth READ imageWidth NOTIFY imageChanged)
    Q_PROPERTY(int imageHeight READ imageHeight NOTIFY imageChanged)
    Q_PROPERTY(bool pastePending READ pastePending NOTIFY pasteChanged)
    Q_PROPERTY(QUrl pasteSource READ pasteSource NOTIFY pasteChanged)
    Q_PROPERTY(int pasteX READ pasteX NOTIFY pasteChanged)
    Q_PROPERTY(int pasteY READ pasteY NOTIFY pasteChanged)
    Q_PROPERTY(int pasteWidth READ pasteWidth NOTIFY pasteChanged)
    Q_PROPERTY(int pasteHeight READ pasteHeight NOTIFY pasteChanged)

public:
    explicit QtAppController(QuickImageProvider* imageProvider = nullptr, QObject* parent = nullptr);

    QString appName() const;
    QString appVersion() const;
    bool english() const;
    QUrl imageSource() const;
    bool hasImage() const;
    bool canUndo() const;
    bool canRedo() const;
    QString imageName() const;
    QString statusText() const;
    QString helpText() const;
    QString exifText() const;
    QString fileInfoText() const;
    int imageWidth() const;
    int imageHeight() const;
    bool pastePending() const;
    QUrl pasteSource() const;
    int pasteX() const;
    int pasteY() const;
    int pasteWidth() const;
    int pasteHeight() const;

    Q_INVOKABLE void openImage(const QUrl& fileUrl);
    Q_INVOKABLE void saveImage(const QUrl& fileUrl);
    Q_INVOKABLE void copyExif();
    Q_INVOKABLE void clearImage();
    Q_INVOKABLE void rotateRight();
    Q_INVOKABLE void rotate180();
    Q_INVOKABLE void rotateLeft();
    Q_INVOKABLE void resizeImage(int width, int height);
    Q_INVOKABLE void cropImage(int x, int y, int width, int height);
    Q_INVOKABLE void convertToFullColor();
    Q_INVOKABLE void convertTo256Colors();
    Q_INVOKABLE void convertToGrayscale();
    Q_INVOKABLE void flipHorizontal();
    Q_INVOKABLE void flipVertical();
    Q_INVOKABLE void copyImage();
    Q_INVOKABLE void copyImageRegion(int x, int y, int width, int height);
    Q_INVOKABLE void resizeImageBy(int width, int height, bool percent, bool keepAspectRatio);
    Q_INVOKABLE void pasteImage();
    Q_INVOKABLE void movePaste(int x, int y);
    Q_INVOKABLE void commitPaste();
    Q_INVOKABLE void retryPaste();
    Q_INVOKABLE void setSaveOptions(int quality, int compression);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void setEnglish(bool enabled);

signals:
    void languageChanged();
    void imageChanged();
    void imageOpened();
    void editHistoryChanged();
    void statusChanged();
    void exifChanged();
    void fileInfoChanged();
    void pasteChanged();

private:
    QString localize(const QString& japanese, const QString& english) const;
    QString readEmbeddedHelp(const QString& resourcePath) const;
    void updateDisplayedImage(const QImage& image, const QString& noticeJapanese, const QString& noticeEnglish);
    void applyImageEdit(const QImage& image, const QString& noticeJapanese, const QString& noticeEnglish);
    void clearEditHistory();
    void clearPasteState();

    bool english_ = false;
    QUrl imageSource_;
    QString imageName_;
    QString sourcePath_;
    ImageEngine::ExifFields exifFields_;
    QString statusText_;
    QuickImageProvider* imageProvider_ = nullptr;
    quint64 imageRevision_ = 0;
    QImage image_;
    QImage pasteImage_;
    int pasteX_ = 0;
    int pasteY_ = 0;
    QUrl pasteSource_;
    ImageEngine::SaveOptions saveOptions_;
    QVector<QImage> undoHistory_;
    QVector<QImage> redoHistory_;
};
