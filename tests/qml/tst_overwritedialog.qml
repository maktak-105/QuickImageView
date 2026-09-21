import QtQuick
import QtTest
import "../../src/ui" as QuickImageView

// 上書きを拒否したことは、状態テキストではなく画面中央の警告で伝える（小さく淡い文字は見落とされていた）。
TestCase {
    name: "OverwriteRefusedDialog"

    Component {
        id: dialogComponent
        QuickImageView.OverwriteRefusedDialog {
            english: false
        }
    }

    SignalSpy { id: acceptedSpy; signalName: "accepted" }

    function test_japanese_message_and_path() {
        const dialog = createTemporaryObject(dialogComponent, null, {path: "C:\\Pictures\\photo.png"})
        verify(dialog.modal)
        verify(findChild(dialog, "overwriteRefusedTitle").text.indexOf("上書きはできません") >= 0)
        const message = findChild(dialog, "overwriteRefusedMessage").text
        verify(message.indexOf("同じ名前のファイルが既にあります") >= 0)
        verify(message.indexOf("既存のファイルは上書きしません") >= 0)
        compare(findChild(dialog, "overwriteRefusedPath").text, "C:\\Pictures\\photo.png")
    }

    function test_english_message() {
        const dialog = createTemporaryObject(dialogComponent, null, {english: true, path: "C:\\Pictures\\photo.png"})
        verify(findChild(dialog, "overwriteRefusedTitle").text.indexOf("Cannot overwrite") >= 0)
        verify(findChild(dialog, "overwriteRefusedMessage").text.indexOf("already exists") >= 0)
    }

    function test_original_image_has_its_own_message() {
        const ja = createTemporaryObject(dialogComponent, null, {original: true})
        verify(findChild(ja, "overwriteRefusedMessage").text.indexOf("元の画像は変更していません") >= 0)
        const en = createTemporaryObject(dialogComponent, null, {english: true, original: true})
        verify(findChild(en, "overwriteRefusedMessage").text.indexOf("original image") >= 0)
    }

    function test_the_path_box_is_hidden_without_a_path() {
        const dialog = createTemporaryObject(dialogComponent, null)
        verify(!findChild(dialog, "overwriteRefusedPath").visible)
    }

    function test_ok_accepts_and_closes() {
        const dialog = createTemporaryObject(dialogComponent, null)
        acceptedSpy.target = dialog
        acceptedSpy.clear()
        findChild(dialog, "overwriteRefusedOkButton").clicked()
        compare(acceptedSpy.count, 1)
    }
}
