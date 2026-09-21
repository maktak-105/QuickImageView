#include "qt_app_controller.h"
#include "image_engine.h"
#include "native_file_dialog.h"
#include "quick_image_provider.h"

#include <QFile>
#include <QFileInfo>
#include <QClipboard>
#include <QGuiApplication>
#include <QImage>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>
#include <QTemporaryDir>


// WIC error codes for "no such encoder": WINCODEC_ERR_COMPONENTNOTFOUND and REGDB_E_CLASSNOTREG.
static bool heifEncoderIsMissing(const QString& error) {
    return error.contains(QStringLiteral("0x88982f50"), Qt::CaseInsensitive) ||
           error.contains(QStringLiteral("0x80040154"), Qt::CaseInsensitive);
}

class QtAppControllerTest final : public QObject {
    Q_OBJECT

private slots:
    void defaultsAreJapanese();
    void languageSwitchUpdatesState();
    void formatsExifTextInJapaneseAndEnglish();
    void languageSwitchUpdatesExifLabels();
    void rejectsNonLocalUrl();
    void rejectsMissingImagePath();
    void loadsPngThroughWic();
    void loadsWebPThroughQtImageFormats();
    void windowSizeDefaultsToSevenTwentyByFourEighty();
    void windowSizeIsSavedAndReadBackAtTheNextStart();
    void windowSizeIsKeptInsideTheAllowedRange();
    void savesTiffWithoutQtImagePlugin();
    void aFailedSaveLeavesNoEmptyFileBehind();
    void everySaveTypeOfTheFileDialogIsWritable();
    void savesHeicWithQualityWhenWicHasAnEncoder();
    void saveAppendsTheSelectedExtensionAndReloadsTheFile();
    void saveWithoutAnyExtensionIsRejected();
    void convertsFilesFromTheCommandLineWithoutOverwriting();
    void rotatesDisplayedImage();
    void rotatesDisplayedImage180Degrees();
    void resizesDisplayedImage();
    void cropsDisplayedImage();
    void convertsDisplayedImageToGrayscale();
    void pasteCanBePositionedAndCommitted();
    void saveOptionsAreApplied();
    void undoAndRedoRestoreImageState();
    void contextMenuRegistrationCanBeQueried();
};

void QtAppControllerTest::defaultsAreJapanese() {
    QtAppController controller;
    QVERIFY(!controller.english());
    QCOMPARE(controller.appName(), QStringLiteral("QuickImageView"));
    QCOMPARE(controller.appVersion(), QStringLiteral("4.1.0"));
    QVERIFY(!controller.hasImage());
    QVERIFY(controller.statusText().contains(QStringLiteral("画像")));
}

void QtAppControllerTest::languageSwitchUpdatesState() {
    QtAppController controller;
    QSignalSpy languageSpy(&controller, &QtAppController::languageChanged);
    QSignalSpy statusSpy(&controller, &QtAppController::statusChanged);

    controller.setEnglish(true);

    QVERIFY(controller.english());
    QCOMPARE(languageSpy.count(), 1);
    QCOMPARE(statusSpy.count(), 1);
    QVERIFY(controller.statusText().contains(QStringLiteral("Open an image")));
}

void QtAppControllerTest::formatsExifTextInJapaneseAndEnglish() {
    ImageEngine::ExifFields fields;
    QCOMPARE(ImageEngine::formatExifText(fields, false), QStringLiteral("EXIF: なし"));
    QCOMPARE(ImageEngine::formatExifText(fields, true), QStringLiteral("EXIF: none"));

    fields.make = QStringLiteral("Canon");
    fields.model = QStringLiteral("EOS");
    fields.taken = QStringLiteral("2026:08:24");
    QCOMPARE(ImageEngine::formatExifText(fields, false),
             QStringLiteral("EXIF: メーカー=Canon  機種=EOS  撮影日時=2026:08:24"));
    QCOMPARE(ImageEngine::formatExifText(fields, true),
             QStringLiteral("EXIF: Make=Canon  Model=EOS  Taken=2026:08:24"));
}

void QtAppControllerTest::languageSwitchUpdatesExifLabels() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString filePath = directory.filePath(QStringLiteral("exif.png"));
    QImage fixture(2, 2, QImage::Format_ARGB32);
    fixture.fill(Qt::red);
    QVERIFY(fixture.save(filePath, "PNG"));

    QtAppController controller;
    controller.openImage(QUrl::fromLocalFile(filePath));
    QVERIFY(controller.hasImage());
    QCOMPARE(controller.exifText(), QStringLiteral("EXIF: なし"));

    QSignalSpy exifSpy(&controller, &QtAppController::exifChanged);
    controller.setEnglish(true);
    QCOMPARE(exifSpy.count(), 1);
    QCOMPARE(controller.exifText(), QStringLiteral("EXIF: none"));
}

void QtAppControllerTest::rejectsNonLocalUrl() {
    QtAppController controller;
    const QUrl assetUrl(QStringLiteral("qrc:/src/app/help/help_jp.md"));

    controller.openImage(assetUrl);

    QVERIFY(!controller.hasImage());
    QVERIFY(controller.statusText().contains(QStringLiteral("ローカル画像")));
}

void QtAppControllerTest::rejectsMissingImagePath() {
    QtAppController controller;
    controller.openImage(QUrl::fromLocalFile(QStringLiteral("C:/missing-quickimageview-test-file.png")));

    QVERIFY(!controller.hasImage());
    QVERIFY(controller.statusText().contains(QStringLiteral("見つかりません")));
}

void QtAppControllerTest::loadsPngThroughWic() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString filePath = directory.filePath(QStringLiteral("sample.png"));
    QImage fixture(3, 2, QImage::Format_ARGB32);
    fixture.fill(qRgba(16, 192, 224, 255));
    QVERIFY(fixture.save(filePath, "PNG"));

    QString error;
    const QImage loaded = ImageEngine::load(filePath, &error);

    QVERIFY2(!loaded.isNull(), qPrintable(error));
    QCOMPARE(loaded.size(), fixture.size());
    QCOMPARE(loaded.pixelColor(0, 0), fixture.pixelColor(0, 0));
}

void QtAppControllerTest::loadsWebPThroughQtImageFormats() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString filePath = directory.filePath(QStringLiteral("sample.webp"));
    QImage fixture(2, 2, QImage::Format_ARGB32);
    fixture.fill(QColor(0, 220, 255, 255));
    QVERIFY(fixture.save(filePath, "WEBP"));

    QString error;
    const QImage loaded = ImageEngine::load(filePath, &error);

    QVERIFY2(!loaded.isNull(), qPrintable(error));
    QCOMPARE(loaded.size(), QSize(2, 2));
    QCOMPARE(loaded.pixelColor(0, 0).red(), 0);
    QCOMPARE(loaded.pixelColor(0, 0).green(), 220);
}

void QtAppControllerTest::windowSizeDefaultsToSevenTwentyByFourEighty() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QtAppController controller;
    controller.setSettingsFile(directory.filePath(QStringLiteral("settings.ini")));
    QCOMPARE(controller.windowWidth(), 720);
    QCOMPARE(controller.windowHeight(), 480);
    QVERIFY(!QFileInfo::exists(directory.filePath(QStringLiteral("settings.ini"))));  // nothing is written by reading
}

void QtAppControllerTest::windowSizeIsSavedAndReadBackAtTheNextStart() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString settingsFile = directory.filePath(QStringLiteral("nested/settings.ini"));

    QtAppController controller;
    controller.setSettingsFile(settingsFile);
    QSignalSpy changed(&controller, &QtAppController::windowSizeChanged);
    QSignalSpy resizeRequested(&controller, &QtAppController::windowResizeRequested);
    controller.setWindowSize(900, 600);
    QCOMPARE(controller.windowWidth(), 900);
    QCOMPARE(controller.windowHeight(), 600);
    QCOMPARE(changed.count(), 1);
    QCOMPARE(resizeRequested.count(), 1);   // the running window is asked to take the new size
    QCOMPARE(resizeRequested.first().at(0).toInt(), 900);
    QCOMPARE(resizeRequested.first().at(1).toInt(), 600);

    const QSettings stored(settingsFile, QSettings::IniFormat);   // the file is a plain, readable ini
    QCOMPARE(stored.value(QStringLiteral("window/width")).toInt(), 900);
    QCOMPARE(stored.value(QStringLiteral("window/height")).toInt(), 600);

    QtAppController nextStart;   // a new start reads the saved size
    nextStart.setSettingsFile(settingsFile);
    QCOMPARE(nextStart.windowWidth(), 900);
    QCOMPARE(nextStart.windowHeight(), 600);
}

void QtAppControllerTest::windowSizeIsKeptInsideTheAllowedRange() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QtAppController controller;
    controller.setSettingsFile(directory.filePath(QStringLiteral("settings.ini")));

    controller.setWindowSize(10, 10);
    QCOMPARE(controller.windowWidth(), QtAppController::kMinimumWindowWidth);
    QCOMPARE(controller.windowHeight(), QtAppController::kMinimumWindowHeight);
    controller.setWindowSize(999999, 999999);
    QCOMPARE(controller.windowWidth(), QtAppController::kMaximumWindowWidth);
    QCOMPARE(controller.windowHeight(), QtAppController::kMaximumWindowHeight);

    // A hand-edited or corrupt file cannot produce an unusable window either.
    {
        QSettings broken(directory.filePath(QStringLiteral("broken.ini")), QSettings::IniFormat);
        broken.setValue(QStringLiteral("window/width"), 5);
        broken.setValue(QStringLiteral("window/height"), QStringLiteral("abc"));
    }
    QtAppController fromBrokenFile;
    fromBrokenFile.setSettingsFile(directory.filePath(QStringLiteral("broken.ini")));
    QCOMPARE(fromBrokenFile.windowWidth(), QtAppController::kMinimumWindowWidth);
    QCOMPARE(fromBrokenFile.windowHeight(), QtAppController::kMinimumWindowHeight);
    // start-up size never drops below the window's minimum
    QVERIFY(fromBrokenFile.startupWindowWidth() >= QtAppController::kMinimumWindowWidth);
    QVERIFY(fromBrokenFile.startupWindowHeight() >= QtAppController::kMinimumWindowHeight);
}

void QtAppControllerTest::savesTiffWithoutQtImagePlugin() {
    // TIFF is written through WIC, so it must work even when Qt has no TIFF image plugin
    // (the MinGW build of Qt ships none).
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QImage fixture(4, 3, QImage::Format_ARGB32);
    fixture.fill(QColor(10, 200, 120, 255));

    for (const QString& name : {QStringLiteral("plain.tif"), QStringLiteral("plain.TIFF")}) {
        for (const int compression : {0, 6}) {
            const QString filePath = directory.filePath(QStringLiteral("c%1_%2").arg(compression).arg(name));
            ImageEngine::SaveOptions options;
            options.compression = compression;
            QString error;
            QVERIFY2(ImageEngine::save(fixture, filePath, options, &error), qPrintable(error));
            QVERIFY(QFileInfo::exists(filePath));

            const QImage loaded = ImageEngine::load(filePath, &error);
            QVERIFY2(!loaded.isNull(), qPrintable(error));
            QCOMPARE(loaded.size(), fixture.size());
            QCOMPARE(loaded.pixelColor(0, 0).green(), 200);
        }
    }
}

void QtAppControllerTest::everySaveTypeOfTheFileDialogIsWritable() {
    // The Save as dialog must not offer a format that ImageEngine cannot write (TIFF once did on MinGW Qt).
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QImage fixture(4, 4, QImage::Format_ARGB32);
    fixture.fill(Qt::blue);

    const auto types = NativeFileDialog::saveFileTypes();
    QVERIFY(!types.isEmpty());
    for (const NativeFileDialog::FileType& type : types) {
        QVERIFY(!type.suffixes.isEmpty());
        for (const QString& suffix : type.suffixes) {
            const QString filePath = directory.filePath(QStringLiteral("x_") + suffix + QLatin1Char('.') + suffix);
            QString error;
            const bool saved = ImageEngine::save(fixture, filePath, ImageEngine::SaveOptions{}, &error);
            // HEIC/HEIF needs a Windows HEIF encoder; only "the component is missing" excuses a failure.
            if (!saved && type.name.startsWith(QLatin1String("HEIC")) && heifEncoderIsMissing(error)) continue;
            QVERIFY2(saved, qPrintable(type.name + " (" + suffix + "): " + error));
        }
    }
}

void QtAppControllerTest::aFailedSaveLeavesNoEmptyFileBehind() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QImage fixture(4, 4, QImage::Format_ARGB32);
    fixture.fill(Qt::red);

    // GIF cannot be written: the failure must not leave an empty file that blocks the next attempt.
    const QString failing = directory.filePath(QStringLiteral("failing.gif"));
    QString error;
    QVERIFY(!ImageEngine::save(fixture, failing, ImageEngine::SaveOptions{}, &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!QFileInfo::exists(failing));

    // An existing file is refused up front and left exactly as it was (the encoders would empty it first).
    const QString existing = directory.filePath(QStringLiteral("existing.gif"));
    QFile file(existing);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("keep me");
    file.close();
    QVERIFY(!ImageEngine::save(fixture, existing, ImageEngine::SaveOptions{}, &error));
    QVERIFY(QFileInfo::exists(existing));
    QCOMPARE(QFileInfo(existing).size(), qint64(7));

    // The command-line conversion goes through the same path.
    const QString source = directory.filePath(QStringLiteral("source.png"));
    QVERIFY(fixture.save(source, "PNG"));
    QVERIFY(!ImageEngine::convertFile(source, directory.filePath(QStringLiteral("converted.gif")), &error));
    QVERIFY(!QFileInfo::exists(directory.filePath(QStringLiteral("converted.gif"))));
}

void QtAppControllerTest::savesHeicWithQualityWhenWicHasAnEncoder() {
    // The quality option must reach the WIC HEIF encoder. Needs a HEIF encoder (HEVC extension) on the PC.
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QImage noise(64, 64, QImage::Format_ARGB32);
    for (int y = 0; y < noise.height(); ++y) {
        for (int x = 0; x < noise.width(); ++x) {
            noise.setPixelColor(x, y, QColor((x * 37 + y * 11) % 256, (x * 13 + y * 59) % 256, (x * y) % 256));
        }
    }
    ImageEngine::SaveOptions low;
    low.quality = 5;
    ImageEngine::SaveOptions high;
    high.quality = 95;
    const QString lowPath = directory.filePath(QStringLiteral("low.heic"));
    const QString highPath = directory.filePath(QStringLiteral("high.heic"));

    QString error;
    if (!ImageEngine::save(noise, lowPath, low, &error)) {
        if (heifEncoderIsMissing(error)) QSKIP(qPrintable(QStringLiteral("No HEIF encoder on this PC: ") + error));
        QFAIL(qPrintable(QStringLiteral("HEIC saving failed although an encoder may exist: ") + error));
    }
    QVERIFY2(ImageEngine::save(noise, highPath, high, &error), qPrintable(error));
    QVERIFY(QFileInfo(lowPath).size() > 0);
    QVERIFY2(QFileInfo(lowPath).size() < QFileInfo(highPath).size(),
             "a higher quality should produce a larger file");
    // What was written is a real HEIC file: decode it again when this PC has a HEIF decoder.
    const QImage decoded = ImageEngine::load(highPath, &error);
    if (!decoded.isNull()) QCOMPARE(decoded.size(), noise.size());
}

void QtAppControllerTest::saveAppendsTheSelectedExtensionAndReloadsTheFile() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString sourcePath = directory.filePath(QStringLiteral("source.png"));
    QImage fixture(8, 6, QImage::Format_ARGB32);
    fixture.fill(Qt::magenta);
    QVERIFY(fixture.save(sourcePath, "PNG"));

    QtAppController controller;
    controller.openImage(QUrl::fromLocalFile(sourcePath));
    QVERIFY(controller.hasImage());
    controller.rotateRight();
    QVERIFY(controller.canUndo());

    // The file name has no extension: the extension of the selected file type (as reported by the dialog) is used.
    controller.saveImage(QUrl::fromLocalFile(directory.filePath(QStringLiteral("result"))), QStringLiteral("*.png"));

    const QString expected = directory.filePath(QStringLiteral("result.png"));
    QVERIFY(QFileInfo::exists(expected));
    // The saved file is reloaded and shown as the current image, without a confirmation.
    QCOMPARE(controller.imageName(), QStringLiteral("result.png"));
    QCOMPARE(controller.imageWidth(), 6);   // rotated 90 degrees: 8x6 -> 6x8
    QCOMPARE(controller.imageHeight(), 8);
    QVERIFY(!controller.canUndo());
    QVERIFY(controller.statusText().contains(QStringLiteral("再読み込み")));
}

void QtAppControllerTest::saveWithoutAnyExtensionIsRejected() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString sourcePath = directory.filePath(QStringLiteral("source.png"));
    QImage fixture(4, 4, QImage::Format_ARGB32);
    fixture.fill(Qt::green);
    QVERIFY(fixture.save(sourcePath, "PNG"));

    QtAppController controller;
    controller.openImage(QUrl::fromLocalFile(sourcePath));

    // No extension in the name and none from the dialog: nothing is written and the image stays.
    controller.saveImage(QUrl::fromLocalFile(directory.filePath(QStringLiteral("noext"))), QString());
    QVERIFY(!QFileInfo::exists(directory.filePath(QStringLiteral("noext"))));
    QCOMPARE(controller.imageName(), QStringLiteral("source.png"));

    // The completed name is checked against the original: "source" + png would overwrite it.
    controller.saveImage(QUrl::fromLocalFile(directory.filePath(QStringLiteral("source"))), QStringLiteral("png"));
    QCOMPARE(controller.imageName(), QStringLiteral("source.png"));
    QVERIFY(controller.statusText().contains(QStringLiteral("原本")));
}

void QtAppControllerTest::convertsFilesFromTheCommandLineWithoutOverwriting() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString sourcePath = directory.filePath(QStringLiteral("source.png"));
    QImage fixture(4, 3, QImage::Format_ARGB32);
    fixture.fill(QColor(200, 30, 60));
    QVERIFY(fixture.save(sourcePath, "PNG"));

    for (const QString& name : {QStringLiteral("out.bmp"), QStringLiteral("out.tif"), QStringLiteral("out.jpg")}) {
        const QString destination = directory.filePath(name);
        QString error;
        QVERIFY2(ImageEngine::convertFile(sourcePath, destination, &error), qPrintable(name + ": " + error));
        const QImage loaded = ImageEngine::load(destination, &error);
        QVERIFY2(!loaded.isNull(), qPrintable(name + ": " + error));
        QCOMPARE(loaded.size(), fixture.size());
    }

    QString error;
    QVERIFY(!ImageEngine::convertFile(sourcePath, sourcePath, &error));               // same path
    QVERIFY(!error.isEmpty());
    const qint64 existingSize = QFileInfo(directory.filePath(QStringLiteral("out.bmp"))).size();
    QVERIFY(!ImageEngine::convertFile(sourcePath, directory.filePath(QStringLiteral("out.bmp")), &error));  // exists
    QCOMPARE(QFileInfo(directory.filePath(QStringLiteral("out.bmp"))).size(), existingSize);
    QVERIFY(!ImageEngine::convertFile(directory.filePath(QStringLiteral("missing.png")),
                                      directory.filePath(QStringLiteral("x.bmp")), &error));  // no source
    QVERIFY(!QFileInfo::exists(directory.filePath(QStringLiteral("x.bmp"))));
}

void QtAppControllerTest::rotatesDisplayedImage() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString filePath = directory.filePath(QStringLiteral("rotate.png"));
    QImage fixture(3, 2, QImage::Format_ARGB32);
    fixture.fill(Qt::red);
    QVERIFY(fixture.save(filePath, "PNG"));

    QuickImageProvider provider;
    QtAppController controller(&provider);
    controller.openImage(QUrl::fromLocalFile(filePath));
    QVERIFY(controller.hasImage());
    const QUrl before = controller.imageSource();

    controller.rotateRight();

    QVERIFY(controller.imageSource() != before);
    QSize imageSize;
    const QImage rotated = provider.requestImage(QString(), &imageSize, QSize());
    QCOMPARE(imageSize, QSize(2, 3));
    QCOMPARE(rotated.size(), QSize(2, 3));
}

void QtAppControllerTest::rotatesDisplayedImage180Degrees() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString filePath = directory.filePath(QStringLiteral("rotate180.png"));
    QImage fixture(2, 2, QImage::Format_ARGB32);
    fixture.setPixelColor(0, 0, Qt::red);
    fixture.setPixelColor(1, 0, Qt::green);
    fixture.setPixelColor(0, 1, Qt::blue);
    fixture.setPixelColor(1, 1, Qt::yellow);
    QVERIFY(fixture.save(filePath, "PNG"));

    QuickImageProvider provider;
    QtAppController controller(&provider);
    controller.openImage(QUrl::fromLocalFile(filePath));
    QVERIFY(controller.hasImage());

    controller.rotate180();

    const QImage rotated = provider.requestImage(QString(), nullptr, QSize());
    QCOMPARE(rotated.size(), QSize(2, 2));
    QCOMPARE(rotated.pixelColor(0, 0), QColor(Qt::yellow));
    QCOMPARE(rotated.pixelColor(1, 0), QColor(Qt::blue));
    QCOMPARE(rotated.pixelColor(0, 1), QColor(Qt::green));
    QCOMPARE(rotated.pixelColor(1, 1), QColor(Qt::red));
    QVERIFY(controller.canUndo());
}

void QtAppControllerTest::resizesDisplayedImage() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString filePath = directory.filePath(QStringLiteral("resize.png"));
    QImage fixture(8, 4, QImage::Format_ARGB32);
    fixture.fill(Qt::red);
    QVERIFY(fixture.save(filePath, "PNG"));

    QuickImageProvider provider;
    QtAppController controller(&provider);
    controller.openImage(QUrl::fromLocalFile(filePath));
    controller.resizeImage(3, 2);

    QCOMPARE(provider.requestImage(QString(), nullptr, QSize()).size(), QSize(3, 2));
    QVERIFY(controller.canUndo());
}

void QtAppControllerTest::cropsDisplayedImage() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString filePath = directory.filePath(QStringLiteral("crop.png"));
    QImage fixture(8, 6, QImage::Format_ARGB32);
    fixture.fill(Qt::red);
    QVERIFY(fixture.save(filePath, "PNG"));

    QuickImageProvider provider;
    QtAppController controller(&provider);
    controller.openImage(QUrl::fromLocalFile(filePath));
    controller.cropImage(2, 1, 4, 3);

    QCOMPARE(provider.requestImage(QString(), nullptr, QSize()).size(), QSize(4, 3));
    QVERIFY(controller.canUndo());
}

void QtAppControllerTest::convertsDisplayedImageToGrayscale() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString filePath = directory.filePath(QStringLiteral("gray.png"));
    QImage fixture(2, 1, QImage::Format_ARGB32);
    fixture.setPixelColor(0, 0, Qt::red);
    fixture.setPixelColor(1, 0, Qt::blue);
    QVERIFY(fixture.save(filePath, "PNG"));

    QuickImageProvider provider;
    QtAppController controller(&provider);
    controller.openImage(QUrl::fromLocalFile(filePath));
    controller.convertToGrayscale();

    const QImage converted = provider.requestImage(QString(), nullptr, QSize());
    QVERIFY(!converted.isNull());
    QCOMPARE(converted.format(), QImage::Format_Grayscale8);
    QVERIFY(controller.canUndo());
}

void QtAppControllerTest::pasteCanBePositionedAndCommitted() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString filePath = directory.filePath(QStringLiteral("paste.png"));
    QImage fixture(8, 8, QImage::Format_ARGB32);
    fixture.fill(Qt::white);
    QVERIFY(fixture.save(filePath, "PNG"));

    QGuiApplication::clipboard()->setImage(QImage(2, 3, QImage::Format_ARGB32));
    QuickImageProvider provider;
    QtAppController controller(&provider);
    controller.openImage(QUrl::fromLocalFile(filePath));
    controller.pasteImage();
    QVERIFY(controller.pastePending());
    QCOMPARE(controller.pasteWidth(), 2);
    QCOMPARE(controller.pasteHeight(), 3);
    controller.movePaste(4, 2);
    QCOMPARE(controller.pasteX(), 4);
    QCOMPARE(controller.pasteY(), 2);
    controller.commitPaste();
    QVERIFY(!controller.pastePending());
    QVERIFY(controller.canUndo());
}

void QtAppControllerTest::saveOptionsAreApplied() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString sourcePath = directory.filePath(QStringLiteral("source.png"));
    const QString outputPath = directory.filePath(QStringLiteral("output.jpg"));
    QImage fixture(16, 16, QImage::Format_ARGB32);
    fixture.fill(Qt::cyan);
    QVERIFY(fixture.save(sourcePath, "PNG"));

    QtAppController controller;
    controller.openImage(QUrl::fromLocalFile(sourcePath));
    controller.setSaveOptions(42, 3);
    controller.saveImage(QUrl::fromLocalFile(outputPath));
    QVERIFY(QFileInfo::exists(outputPath));
    QVERIFY(QFileInfo(outputPath).size() > 0);
}

void QtAppControllerTest::undoAndRedoRestoreImageState() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString filePath = directory.filePath(QStringLiteral("history.png"));
    QImage fixture(4, 2, QImage::Format_ARGB32);
    fixture.fill(Qt::cyan);
    QVERIFY(fixture.save(filePath, "PNG"));

    QuickImageProvider provider;
    QtAppController controller(&provider);
    controller.openImage(QUrl::fromLocalFile(filePath));
    controller.rotateRight();
    QVERIFY(controller.canUndo());
    QVERIFY(!controller.canRedo());

    controller.undo();
    QVERIFY(!controller.canUndo());
    QVERIFY(controller.canRedo());
    QVERIFY(provider.requestImage(QString(), nullptr, QSize()).size() == QSize(4, 2));

    controller.redo();
    QVERIFY(controller.canUndo());
    QVERIFY(!controller.canRedo());
    QVERIFY(provider.requestImage(QString(), nullptr, QSize()).size() == QSize(2, 4));
}

void QtAppControllerTest::contextMenuRegistrationCanBeQueried() {
    QtAppController controller;
    QSignalSpy spy(&controller, &QtAppController::contextMenuRegisteredChanged);
    QVERIFY(spy.isValid());
    // isContextMenuRegistered() returns a boolean representing the current registry state
    const bool isRegistered = controller.isContextMenuRegistered();
    QCOMPARE(controller.property("contextMenuRegistered").toBool(), isRegistered);
}

QTEST_MAIN(QtAppControllerTest)
#include "tst_qt_app_controller.moc"
