#pragma once

#include <QImage>
#include <QString>

class ImageEngine final {
public:
    struct SaveOptions {
        int quality = 90;
        int compression = 6;
    };

    struct ExifFields {
        QString make;
        QString model;
        QString taken;
        bool isEmpty() const { return make.isEmpty() && model.isEmpty() && taken.isEmpty(); }
    };

    static QImage load(const QString& filePath, QString* errorMessage);
    static ExifFields readExif(const QString& filePath);
    static QString formatExifText(const ExifFields& fields, bool english);
    static bool save(const QImage& image, const QString& filePath, QString* errorMessage);
    static bool save(const QImage& image, const QString& filePath, const SaveOptions& options,
                     QString* errorMessage);
    static QImage resize(const QImage& image, int width, int height);
    static QImage convertColor(const QImage& image, int mode);
};
