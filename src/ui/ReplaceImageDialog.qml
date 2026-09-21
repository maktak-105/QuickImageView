import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 画像を表示中に別の画像がドロップされたときの置換確認。3.x と同じく、確認してから開く。
Dialog {
    id: replaceDialog
    objectName: "replaceImageDialog"
    modal: true
    dim: true
    anchors.centerIn: Overlay.overlay
    // ウィンドウより大きくならない（既定 720x480、最小 480x320 でも収まる）。はみ出す分はスクロールする。
    readonly property real maxDialogWidth: Overlay.overlay ? Overlay.overlay.width - 32 : 460
    readonly property real maxDialogHeight: Overlay.overlay ? Overlay.overlay.height - 32 : 200
    width: Math.min(460, maxDialogWidth)
    height: Math.min(implicitHeight, maxDialogHeight)
    padding: 24

    required property bool english

    background: Rectangle {
        radius: 14
        color: "#111924"
        border.color: "#00a9c8"
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 14

        Label {
            objectName: "replaceImageTitle"
            text: replaceDialog.english ? "Open image" : "画像を開く確認"
            color: "#e7edf5"
            font.pixelSize: 19
            font.bold: true
        }

        Label {
            objectName: "replaceImageMessage"
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: replaceDialog.english
                ? "Close the current image and open the dropped image?"
                : "現在開いている画像を閉じて、ドロップした画像を開きますか？"
            color: "#e7edf5"
            font.pixelSize: 14
        }
    }

    footer: DialogButtonBox {
        padding: 16
        alignment: Qt.AlignRight
        background: Item {}

        Button {
            objectName: "replaceImageYesButton"
            Accessible.name: replaceDialog.english ? "Open the dropped image" : "ドロップした画像を開く"
            text: replaceDialog.english ? "Yes" : "はい"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }

        Button {
            objectName: "replaceImageNoButton"
            Accessible.name: replaceDialog.english ? "Keep the current image" : "現在の画像を残す"
            text: replaceDialog.english ? "No" : "いいえ"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
