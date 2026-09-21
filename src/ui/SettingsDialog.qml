import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: settingsDialog
    objectName: "settingsDialog"
    modal: true
    dim: true
    anchors.centerIn: Overlay.overlay
    width: 460
    padding: 24

    required property bool english

    background: Rectangle {
        radius: 14
        color: "#111924"
        border.color: "#00a9c8"
        border.width: 1
    }

    onAboutToShow: {
        contextMenuCheckBox.checked = appController.contextMenuRegistered
    }

    contentItem: ColumnLayout {
        spacing: 14

        Label {
            text: settingsDialog.english ? "Settings" : "設定"
            color: "#e7edf5"
            font.pixelSize: 19
            font.bold: true
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#223246"
            Layout.topMargin: 2
            Layout.bottomMargin: 6
        }

        Label {
            text: settingsDialog.english ? "[Explorer Integration]" : "[エクスプローラー連携]"
            color: "#00c9e8"
            font.bold: true
        }

        CheckBox {
            id: contextMenuCheckBox
            objectName: "contextMenuCheckBox"
            Accessible.name: settingsDialog.english ? "Add to context menu" : "右クリックメニューに追加"
            text: settingsDialog.english ? "Add to context menu" : "右クリックメニューに追加"
            font.pixelSize: 14

            contentItem: Text {
                text: contextMenuCheckBox.text
                font: contextMenuCheckBox.font
                color: "#e7edf5"
                verticalAlignment: Text.AlignVCenter
                leftPadding: contextMenuCheckBox.indicator.width + 8
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 28
            text: settingsDialog.english
                ? "Show QuickImageView with an icon in the Windows Explorer right-click menu for image files."
                : "Windows エクスプローラーで画像ファイルを右クリックしたときに、アイコン付きでQuickImageViewを表示します。"
            color: "#91a0b3"
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 10
        }
    }

    footer: DialogButtonBox {
        padding: 16
        alignment: Qt.AlignRight
        background: Item {}

        Button {
            id: okButton
            objectName: "settingsOkButton"
            Accessible.name: settingsDialog.english ? "Save settings" : "設定を保存"
            text: "OK"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                appController.setContextMenuRegistered(contextMenuCheckBox.checked)
                settingsDialog.close()
            }
        }

        Button {
            id: cancelButton
            objectName: "settingsCancelButton"
            Accessible.name: settingsDialog.english ? "Cancel settings" : "設定をキャンセル"
            text: settingsDialog.english ? "Cancel" : "キャンセル"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: settingsDialog.close()
        }
    }
}

