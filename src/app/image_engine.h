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
    // Command-line conversion: never overwrites the source or an existing destination.
    static bool convertFile(const QString& sourcePath, const QString& destinationPath,
                            QString* errorMessage);
    static QImage resize(const QImage& image, int width, int height);

private:
    static bool saveWithoutCleanup(const QImage& image, const QString& filePath, const SaveOptions& options,
                                   QString* errorMessage);

public:
    static QImage convertColor(const QImage& image, int mode);
};
