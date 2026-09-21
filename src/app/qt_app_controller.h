#pragma once

#include <QObject>
#include <QImage>
#include <QSize>
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
    Q_PROPERTY(bool contextMenuRegistered READ isContextMenuRegistered NOTIFY contextMenuRegisteredChanged)
    // Window size (client area, logical pixels): the saved value, the size used at start-up, and the allowed range.
    Q_PROPERTY(int windowWidth READ windowWidth NOTIFY windowSizeChanged)
    Q_PROPERTY(int windowHeight READ windowHeight NOTIFY windowSizeChanged)
    Q_PROPERTY(int startupWindowWidth READ startupWindowWidth CONSTANT)
    Q_PROPERTY(int startupWindowHeight READ startupWindowHeight CONSTANT)
    Q_PROPERTY(int minimumWindowWidth READ minimumWindowWidth CONSTANT)
    Q_PROPERTY(int minimumWindowHeight READ minimumWindowHeight CONSTANT)
    Q_PROPERTY(int maximumWindowWidth READ maximumWindowWidth CONSTANT)
    Q_PROPERTY(int maximumWindowHeight READ maximumWindowHeight CONSTANT)

public:
    static constexpr int kDefaultWindowWidth = 720;
    static constexpr int kDefaultWindowHeight = 480;
    static constexpr int kMinimumWindowWidth = 480;
    static constexpr int kMinimumWindowHeight = 320;
    static constexpr int kMaximumWindowWidth = 7680;
    static constexpr int kMaximumWindowHeight = 4320;

    explicit QtAppController(QuickImageProvider* imageProvider = nullptr, QObject* parent = nullptr);

    // Keeps a window size inside the allowed range.
    static QSize clampWindowSize(const QSize& size);
    // Where the settings are stored (settings.ini). Tests point it at a temporary file.
    void setSettingsFile(const QString& path);
    int windowWidth() const;
    int windowHeight() const;
    int startupWindowWidth() const;
    int startupWindowHeight() const;
    int minimumWindowWidth() const { return kMinimumWindowWidth; }
    int minimumWindowHeight() const { return kMinimumWindowHeight; }
    int maximumWindowWidth() const { return kMaximumWindowWidth; }
    int maximumWindowHeight() const { return kMaximumWindowHeight; }

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
    // defaultSuffix: extension of the format chosen in the file dialog, used when the name has none.
    Q_INVOKABLE void saveImage(const QUrl& fileUrl, const QString& defaultSuffix);
    // Windows common file dialogs; they open / save through openImage() and saveImage().
    Q_INVOKABLE void showOpenImageDialog();
    Q_INVOKABLE void showSaveImageDialog();
    // Folder where the Save as dialog opens: the folder of the image on screen (empty without a file).
    QString initialSaveFolder() const;
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
    Q_INVOKABLE bool isContextMenuRegistered() const;
    Q_INVOKABLE bool setContextMenuRegistered(bool enable);
    // Saves the size (used at the next start) and asks the window to take it now.
    Q_INVOKABLE void setWindowSize(int width, int height);

signals:
    void languageChanged();
    void imageChanged();
    void imageOpened();
    void editHistoryChanged();
    void statusChanged();
    void exifChanged();
    void fileInfoChanged();
    void pasteChanged();
    void contextMenuRegisteredChanged();
    // A save was refused because the destination exists (isOriginal: it is the image's own file).
    void overwriteRefused(const QString& path, bool isOriginal);
    void windowSizeChanged();
    void windowResizeRequested(int width, int height);

private:
    QString localize(const QString& japanese, const QString& english) const;
    QString readEmbeddedHelp(const QString& resourcePath) const;
    void updateDisplayedImage(const QImage& image, const QString& noticeJapanese, const QString& noticeEnglish);
    void applyImageEdit(const QImage& image, const QString& noticeJapanese, const QString& noticeEnglish);
    void clearEditHistory();
    void clearPasteState();
    void loadWindowSize();

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
    QString settingsFile_;
    QSize windowSize_{kDefaultWindowWidth, kDefaultWindowHeight};
};
