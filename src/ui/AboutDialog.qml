import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: aboutDialog
    objectName: "aboutDialog"
    modal: true
    dim: true
    anchors.centerIn: Overlay.overlay
    // ウィンドウより大きくならない（既定 720x480、最小 480x320 でも収まる）。はみ出す分はスクロールする。
    readonly property real maxDialogWidth: Overlay.overlay ? Overlay.overlay.width - 32 : 380
    readonly property real maxDialogHeight: Overlay.overlay ? Overlay.overlay.height - 32 : 510
    width: Math.min(380, maxDialogWidth)
    height: Math.min(510, maxDialogHeight)
    padding: 24

    required property bool english
    required property string appVersion

    background: Rectangle {
        radius: 14
        color: "#111924"
        border.color: "#00a9c8"
        border.width: 1
    }

    contentItem: ScrollView {
        id: aboutScroller
        clip: true
        contentWidth: availableWidth
        ColumnLayout {
        width: aboutScroller.availableWidth
        spacing: 10
        Label { text: "QuickImageView"; color: "#e7edf5"; font.pixelSize: 19; font.bold: true }
        Label { text: "Ver. " + aboutDialog.appVersion; color: "#00c9e8"; font.bold: true }
        Rectangle { Layout.fillWidth: true; height: 1; color: "#223246"; Layout.topMargin: 2; Layout.bottomMargin: 4 }
        Label { text: aboutDialog.english ? "[Development environment]" : "[開発環境]"; color: "#00c9e8"; font.bold: true }
        Label {
            Layout.fillWidth: true
            text: aboutDialog.english
                ? "Qt 6 / Qt Quick / QML\nC++17 / Windows Imaging Component\nQt Image Formats"
                : "Qt 6 / Qt Quick / QML\nC++17 / Windows Imaging Component\nQt Image Formats"
            color: "#d6dee8"
            wrapMode: Text.Wrap
        }
        Label { text: aboutDialog.english ? "[Author]" : "[制作者]"; color: "#00c9e8"; font.bold: true; Layout.topMargin: 4 }
        Label { text: "GitHub: maktak-105"; color: "#d6dee8" }
        Image {
            objectName: "creatorBadgeImage"
            Accessible.name: aboutDialog.english ? "Creator badge" : "制作者ワッペン"
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 6
            source: "qrc:/assets/maktak105-V04-01.jpg"
            Layout.maximumWidth: 170
            Layout.maximumHeight: 170
            width: 170
            height: 170
            fillMode: Image.PreserveAspectFit
        }
        }
    }

    footer: DialogButtonBox {
        padding: 16
        alignment: Qt.AlignHCenter
        background: Item {}
        Button {
            objectName: "aboutOkButton"
            Accessible.name: aboutDialog.english ? "Close About dialog" : "バージョン情報を閉じる"
            text: "OK"
            onClicked: aboutDialog.close()
        }
    }
}
