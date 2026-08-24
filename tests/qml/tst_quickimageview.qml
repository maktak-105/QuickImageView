import QtQuick
import QtTest
import "../../qml" as QuickImageView

TestCase {
    name: "QuickImageViewQmlSmoke"

    Component {
        id: aboutComponent
        QuickImageView.AboutDialog {
            english: false
            appVersion: "3.1.2"
        }
    }

    function test_qml_test_infrastructure_is_available() {
        const about = createTemporaryObject(aboutComponent, null)
        verify(about !== null)
        compare(about.objectName, "aboutDialog")
        verify(about.modal)
        compare(about.appVersion, "3.1.2")
    }
}
