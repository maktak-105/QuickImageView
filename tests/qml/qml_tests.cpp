#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QTemporaryDir>
#include <QtQuickTest>

#include "qt_app_controller.h"

// The dialogs read the global "appController" exactly like the application. The tests give them the real
// controller with a temporary settings file, so nothing of the user's settings is read or written.
class Setup final : public QObject {
    Q_OBJECT

public:
    Setup() { QQuickStyle::setStyle(QStringLiteral("Basic")); }

public slots:
    void qmlEngineAvailable(QQmlEngine* engine) {
        auto* controller = new QtAppController(nullptr, engine);
        controller->setSettingsFile(directory_.filePath(QStringLiteral("settings.ini")));
        engine->rootContext()->setContextProperty(QStringLiteral("appController"), controller);
    }

private:
    QTemporaryDir directory_;
};

QUICK_TEST_MAIN_WITH_SETUP(qiv_qml_tests, Setup)

#include "qml_tests.moc"
