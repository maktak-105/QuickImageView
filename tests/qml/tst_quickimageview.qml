import QtQuick
import QtTest
import "../../src/ui" as QuickImageView

TestCase {
    name: "QuickImageViewQmlSmoke"

    Component {
        id: aboutComponent
        QuickImageView.AboutDialog {
            english: false
            appVersion: "4.2.0"
        }
    }

    function test_qml_test_infrastructure_is_available() {
        const about = createTemporaryObject(aboutComponent, null)
        verify(about !== null)
        compare(about.objectName, "aboutDialog")
        verify(about.modal)
        compare(about.appVersion, "4.2.0")
    }
}
