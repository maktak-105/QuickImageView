import QtQuick
import QtTest
import "../../src/ui" as QuickImageView

// 画像表示中のドロップは、確認してから開く（3.x と同じ）。
TestCase {
    name: "ReplaceImageDialog"

    Component {
        id: dialogComponent
        QuickImageView.ReplaceImageDialog {
            english: false
        }
    }

    SignalSpy { id: acceptedSpy; signalName: "accepted" }
    SignalSpy { id: rejectedSpy; signalName: "rejected" }

    function test_message_follows_the_language() {
        const ja = createTemporaryObject(dialogComponent, null, {english: false})
        const en = createTemporaryObject(dialogComponent, null, {english: true})
        const jaMessage = findChild(ja, "replaceImageMessage")
        const enMessage = findChild(en, "replaceImageMessage")
        verify(jaMessage !== null && enMessage !== null)
        verify(jaMessage.text.indexOf("ドロップ") >= 0)
        verify(enMessage.text.indexOf("dropped") >= 0)
        compare(findChild(ja, "replaceImageYesButton").text, "はい")
        compare(findChild(en, "replaceImageNoButton").text, "No")
    }

    function test_yes_accepts() {
        const dialog = createTemporaryObject(dialogComponent, null)
        acceptedSpy.target = dialog
        acceptedSpy.clear()
        rejectedSpy.target = dialog
        rejectedSpy.clear()
        findChild(dialog, "replaceImageYesButton").clicked()
        compare(acceptedSpy.count, 1)
        compare(rejectedSpy.count, 0)
    }

    function test_no_rejects_and_does_not_accept() {
        const dialog = createTemporaryObject(dialogComponent, null)
        acceptedSpy.target = dialog
        acceptedSpy.clear()
        rejectedSpy.target = dialog
        rejectedSpy.clear()
        findChild(dialog, "replaceImageNoButton").clicked()
        compare(rejectedSpy.count, 1)
        compare(acceptedSpy.count, 0)
    }
}
