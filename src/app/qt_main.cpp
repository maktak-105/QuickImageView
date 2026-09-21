#include "qt_app_controller.h"
#include "quick_image_provider.h"

#include <QGuiApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QMessageLogContext>
#include <QStandardPaths>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <windows.h>

namespace {
QFile crashLog;
QString crashLogPath;

void writeQtMessage(QtMsgType type, const QMessageLogContext&, const QString& message) {
    if (!crashLog.isOpen()) return;
    const char* level = type == QtCriticalMsg ? "critical" : type == QtFatalMsg ? "fatal" :
                        type == QtWarningMsg ? "warning" : "info";
    const QByteArray line = QStringLiteral("%1 [%2] %3\n")
        .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate), QString::fromLatin1(level), message)
        .toUtf8();
    crashLog.write(line);
    crashLog.flush();
}

LONG WINAPI writeCrashRecord(EXCEPTION_POINTERS* exceptionInfo) {
    QFile file(crashLogPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        const quintptr address = reinterpret_cast<quintptr>(exceptionInfo && exceptionInfo->ExceptionRecord
            ? exceptionInfo->ExceptionRecord->ExceptionAddress : nullptr);
        file.write(QStringLiteral("%1 [unhandled exception] code=0x%2 address=0x%3\n")
            .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODate))
            .arg(exceptionInfo && exceptionInfo->ExceptionRecord
                     ? exceptionInfo->ExceptionRecord->ExceptionCode : 0, 8, 16, QLatin1Char('0'))
            .arg(address, QT_POINTER_SIZE * 2, 16, QLatin1Char('0')).toUtf8());
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
}

int main(int argc, char* argv[]) {
    QGuiApplication application(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("maktak-105"));
    QGuiApplication::setApplicationName(QStringLiteral("QuickImageView"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QCoreApplication::setApplicationVersion(QStringLiteral(QUICKIMAGEVIEW_VERSION));
    const QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(logDir);
    crashLogPath = QDir(logDir).filePath(QStringLiteral("crash.log"));
    crashLog.setFileName(crashLogPath);
    (void)crashLog.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    qInstallMessageHandler(writeQtMessage);
    SetUnhandledExceptionFilter(writeCrashRecord);

    QQmlApplicationEngine engine;
    auto* imageProvider = new QuickImageProvider;
    engine.addImageProvider(QStringLiteral("quickimage"), imageProvider);
    QtAppController controller(imageProvider);
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &controller);
    engine.loadFromModule("QuickImageView", "Main");
    if (engine.rootObjects().isEmpty()) return 1;

    if (application.arguments().size() > 1) {
        const QString candidate = application.arguments().at(1);
        if (QFileInfo::exists(candidate)) controller.openImage(QUrl::fromLocalFile(candidate));
    }

    return application.exec();
}
