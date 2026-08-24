#pragma once

#include <QImage>
#include <QString>

class ImageEngine final {
public:
    struct SaveOptions {
        int quality = 90;
        int compression = 6;
    };

    static QImage load(const QString& filePath, QString* errorMessage);
    static QString exifText(const QString& filePath);
    static bool save(const QImage& image, const QString& filePath, QString* errorMessage);
    static bool save(const QImage& image, const QString& filePath, const SaveOptions& options,
                     QString* errorMessage);
    static QImage resize(const QImage& image, int width, int height);
    static QImage convertColor(const QImage& image, int mode);
};
