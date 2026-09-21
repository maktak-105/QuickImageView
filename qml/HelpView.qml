import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: helpDialog
    objectName: "helpDialog"
    modal: true
    dim: true
    anchors.centerIn: Overlay.overlay
    width: 760
    height: 620
    padding: 0

    required property bool english
    required property string helpText

    background: Rectangle { radius: 14; color: "#111924"; border.color: "#00a9c8" }

    header: Label {
        padding: 20
        text: helpDialog.english ? "QuickImageView Help" : "QuickImageView ヘルプ"
        color: "#e7edf5"
        font.bold: true
        font.pixelSize: 18
    }

    contentItem: ScrollView {
        clip: true
        TextArea {
            objectName: "helpTextArea"
            Accessible.name: helpDialog.english ? "Help content" : "ヘルプ本文"
            readOnly: true
            text: helpDialog.helpText
            textFormat: TextEdit.MarkdownText
            color: "#d6dee8"
            background: Rectangle { color: "transparent" }
            wrapMode: TextEdit.Wrap
            padding: 20
        }
    }

    footer: DialogButtonBox {
        padding: 16
        alignment: Qt.AlignHCenter
        background: Item {}
        Button {
            objectName: "helpCloseButton"
            Accessible.name: helpDialog.english ? "Close Help" : "ヘルプを閉じる"
            text: helpDialog.english ? "Close" : "閉じる"
            onClicked: helpDialog.close()
        }
    }
}
