import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 保存先に同名のファイルが既にある（または元画像と同じ場所）ため保存を拒否したことを、画面中央の警告で知らせる。
// 状態テキストは小さく淡いので、上書き禁止は見落とされていた。
Dialog {
    id: overwriteDialog
    objectName: "overwriteRefusedDialog"
    modal: true
    dim: true
    anchors.centerIn: Overlay.overlay
    // ウィンドウより大きくならない（既定 720x480、最小 480x320 でも収まる）。はみ出す分はスクロールする。
    readonly property real maxDialogWidth: Overlay.overlay ? Overlay.overlay.width - 32 : 480
    readonly property real maxDialogHeight: Overlay.overlay ? Overlay.overlay.height - 32 : 300
    width: Math.min(480, maxDialogWidth)
    height: Math.min(implicitHeight, maxDialogHeight)
    padding: 24

    required property bool english
    // 保存しようとした場所（ファイルのフルパス）
    property string path: ""
    // true: 元画像と同じ場所への保存。false: 同名のファイルが既にある。
    property bool original: false

    readonly property color warningColor: "#ffb020"

    background: Rectangle {
        radius: 14
        color: "#111924"
        border.color: overwriteDialog.warningColor
        border.width: 2
    }

    contentItem: ScrollView {
        id: overwriteScroller
        clip: true
        contentWidth: availableWidth
        ColumnLayout {
        width: overwriteScroller.availableWidth
        spacing: 12

        Label {
            objectName: "overwriteRefusedTitle"
            Layout.fillWidth: true
            text: "⚠  " + (overwriteDialog.english ? "Cannot overwrite a file" : "上書きはできません")
            color: overwriteDialog.warningColor
            font.pixelSize: 22
            font.bold: true
            wrapMode: Text.Wrap
        }

        Label {
            objectName: "overwriteRefusedMessage"
            Layout.fillWidth: true
            text: overwriteDialog.original
                ? (overwriteDialog.english
                    ? "The original image cannot be overwritten. It was not changed. Choose another name, format, or folder and save again."
                    : "元の画像と同じ場所には保存できません。元の画像は変更していません。別の名前、別の形式、または別のフォルダーを指定して、もう一度保存してください。")
                : (overwriteDialog.english
                    ? "A file with this name already exists. Existing files are never overwritten. Choose another name or folder and save again."
                    : "同じ名前のファイルが既にあります。既存のファイルは上書きしません。別の名前、または別のフォルダーを指定して、もう一度保存してください。")
            color: "#e7edf5"
            font.pixelSize: 15
            wrapMode: Text.Wrap
        }

        Rectangle {
            Layout.fillWidth: true
            visible: overwriteDialog.path.length > 0
            implicitHeight: pathLabel.implicitHeight + 16
            radius: 8
            color: "#0b1018"
            border.color: "#223246"

            Label {
                id: pathLabel
                objectName: "overwriteRefusedPath"
                anchors.fill: parent
                anchors.margins: 8
                text: overwriteDialog.path
                color: "#d6dee8"
                font.pixelSize: 13
                wrapMode: Text.WrapAnywhere
            }
        }
        }
    }

    footer: DialogButtonBox {
        padding: 16
        alignment: Qt.AlignHCenter
        background: Item {}

        Button {
            objectName: "overwriteRefusedOkButton"
            Accessible.name: overwriteDialog.english ? "Close the warning" : "警告を閉じる"
            text: "OK"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }
}
