import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

// ファイル情報とEXIF情報を常時表示するフローティングウィンドウ。
// Win32版のEXIFウィンドウと同じく、本体の右隣に寄せ、入りきらない場合は左へ回り込ませる。
Window {
    id: infoWindow

    property Window host: null
    property bool english: false
    property string fileInfoText: ""
    property string exifText: ""
    property color surfaceColor: "#111924"
    property color borderColor: "#223246"
    property color textColor: "#e7edf5"
    property color mutedTextColor: "#91a0b3"
    property color raisedSurfaceColor: "#151f2c"

    signal copyExifRequested()

    readonly property int gap: 12

    objectName: "infoWindow"
    title: english ? "File and EXIF information" : "ファイル情報 / EXIF情報"
    width: 440
    height: 260
    minimumWidth: 320
    minimumHeight: 200
    color: "#0b1018"
    // ツールウィンドウにして、本体の操作中も前面に残しつつタスクバーへ出さない。
    flags: Qt.Tool | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint

    x: {
        if (!host) return 0
        const right = host.x + host.width + gap
        if (right + width <= Screen.virtualX + Screen.desktopAvailableWidth) return right
        return Math.max(Screen.virtualX, host.x - width - gap)
    }
    y: host ? Math.max(Screen.virtualY, host.y + 40) : 0

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Label {
            text: infoWindow.english ? "File information" : "ファイル情報"
            color: infoWindow.mutedTextColor
            font.bold: true
            font.pixelSize: 12
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 96
            color: infoWindow.surfaceColor
            border.color: infoWindow.borderColor
            radius: 6

            Flickable {
                anchors.fill: parent
                anchors.margins: 8
                contentWidth: width
                contentHeight: fileInfoLabel.implicitHeight
                clip: true
                Label {
                    id: fileInfoLabel
                    objectName: "fileInfoLabel"
                    Accessible.name: infoWindow.english ? "File information" : "ファイル情報"
                    width: parent.width
                    text: infoWindow.fileInfoText
                    color: infoWindow.textColor
                    wrapMode: Text.Wrap
                    font.pixelSize: 13
                }
            }
        }

        Label {
            text: infoWindow.english ? "EXIF information" : "EXIF情報"
            color: infoWindow.mutedTextColor
            font.bold: true
            font.pixelSize: 12
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 64
            color: infoWindow.surfaceColor
            border.color: infoWindow.borderColor
            radius: 6

            Flickable {
                anchors.fill: parent
                anchors.margins: 8
                contentWidth: width
                contentHeight: exifLabel.implicitHeight
                clip: true
                Label {
                    id: exifLabel
                    objectName: "exifInfoLabel"
                    Accessible.name: infoWindow.english ? "EXIF information" : "EXIF情報"
                    width: parent.width
                    text: infoWindow.exifText
                    color: infoWindow.textColor
                    wrapMode: Text.Wrap
                    font.pixelSize: 13
                }
            }
        }

        Button {
            id: copyExifButton
            objectName: "copyExifButton"
            Layout.alignment: Qt.AlignLeft
            text: infoWindow.english ? "Copy EXIF" : "EXIFをコピー"
            onClicked: infoWindow.copyExifRequested()
            contentItem: Text {
                text: copyExifButton.text
                color: infoWindow.textColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                implicitWidth: 130
                implicitHeight: 30
                radius: 6
                color: copyExifButton.hovered ? "#1c3042" : infoWindow.raisedSurfaceColor
                border.color: infoWindow.borderColor
            }
        }
    }
}
