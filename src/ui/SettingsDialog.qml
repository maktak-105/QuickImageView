import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: settingsDialog
    objectName: "settingsDialog"
    modal: true
    dim: true
    anchors.centerIn: Overlay.overlay
    // ウィンドウより大きくならない（既定 720x480、最小 480x320 でも収まる）。はみ出す分はスクロールする。
    readonly property real maxDialogWidth: Overlay.overlay ? Overlay.overlay.width - 32 : 460
    readonly property real maxDialogHeight: Overlay.overlay ? Overlay.overlay.height - 32 : 700
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

    onAboutToShow: {
        contextMenuCheckBox.checked = appController.contextMenuRegistered
        windowWidthSpinBox.value = appController.windowWidth
        windowHeightSpinBox.value = appController.windowHeight
    }

    contentItem: ScrollView {
        id: settingsScroller
        clip: true
        contentWidth: availableWidth
        ColumnLayout {
        width: settingsScroller.availableWidth
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

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#223246"
            Layout.topMargin: 6
            Layout.bottomMargin: 6
        }

        Label {
            text: settingsDialog.english ? "[Window]" : "[ウィンドウ]"
            color: "#00c9e8"
            font.bold: true
        }

        GridLayout {
            columns: 2
            columnSpacing: 14
            rowSpacing: 8

            Label {
                text: settingsDialog.english ? "Width (px)" : "幅（px）"
                color: "#e7edf5"
            }
            SpinBox {
                id: windowWidthSpinBox
                objectName: "windowWidthSpinBox"
                Accessible.name: settingsDialog.english ? "Window width" : "ウィンドウの幅"
                from: appController.minimumWindowWidth
                to: appController.maximumWindowWidth
                stepSize: 10
                editable: true
            }

            Label {
                text: settingsDialog.english ? "Height (px)" : "高さ（px）"
                color: "#e7edf5"
            }
            SpinBox {
                id: windowHeightSpinBox
                objectName: "windowHeightSpinBox"
                Accessible.name: settingsDialog.english ? "Window height" : "ウィンドウの高さ"
                from: appController.minimumWindowHeight
                to: appController.maximumWindowHeight
                stepSize: 10
                editable: true
            }
        }

        Label {
            Layout.fillWidth: true
            text: settingsDialog.english
                ? "The size of the window's content area. It is used the next time QuickImageView starts and is applied to the current window."
                : "ウィンドウの内側（表示領域）の大きさです。次回の起動時から使い、いまのウィンドウにも反映します。"
            color: "#91a0b3"
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 10
        }
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
                // 入力欄に打ち込んだ値が確定前でも取り込む。変更が無ければ、手で変えた現在の大きさを崩さない。
                const typedWidth = windowWidthSpinBox.valueFromText(windowWidthSpinBox.contentItem.text, windowWidthSpinBox.locale)
                const typedHeight = windowHeightSpinBox.valueFromText(windowHeightSpinBox.contentItem.text, windowHeightSpinBox.locale)
                const width = isNaN(typedWidth) ? windowWidthSpinBox.value : typedWidth
                const height = isNaN(typedHeight) ? windowHeightSpinBox.value : typedHeight
                if (width !== appController.windowWidth || height !== appController.windowHeight) {
                    appController.setWindowSize(width, height)
                }
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

