#include "qt_app_controller.h"
#include "image_engine.h"
#include "quick_image_provider.h"

#include <QFile>
#include <QFileInfo>
#include <QClipboard>
#include <QGuiApplication>
#include <QImage>
#include <QSignalSpy>
#include <QTest>
#include <QTemporaryDir>


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
    void rotatesDisplayedImage();
    void rotatesDisplayedImage180Degrees();
    void resizesDisplayedImage();
    void cropsDisplayedImage();
    void convertsDisplayedImageToGrayscale();
    void pasteCanBePositionedAndCommitted();
    void saveOptionsAreApplied();
    void undoAndRedoRestoreImageState();
};

void QtAppControllerTest::defaultsAreJapanese() {
    QtAppController controller;
    QVERIFY(!controller.english());
    QCOMPARE(controller.appName(), QStringLiteral("QuickImageView"));
    QCOMPARE(controller.appVersion(), QStringLiteral("3.1.2"));
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
    const QUrl assetUrl(QStringLiteral("qrc:/resources/help/help_jp.md"));

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


QTEST_MAIN(QtAppControllerTest)
#include "tst_qt_app_controller.moc"
