import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: window
    objectName: "quickImageViewWindow"
    width: 1180
    height: 760
    minimumWidth: 860
    minimumHeight: 560
    visible: true
    title: appController.appName + " " + appController.appVersion
    color: "#0b1018"

    readonly property color backgroundColor: "#0b1018"
    readonly property color surfaceColor: "#111924"
    readonly property color raisedSurfaceColor: "#151f2c"
    readonly property color borderColor: "#223246"
    readonly property color textColor: "#e7edf5"
    readonly property color mutedTextColor: "#91a0b3"
    readonly property color accentColor: "#00c9e8"
    readonly property color accentDarkColor: "#0086b9"

    // 表示倍率と平行移動。画像そのものではなく表示状態なのでQML側で保持する。
    property real zoom: 1.0
    property real panX: 0
    property real panY: 0

    // 選択範囲は画像座標で保持する。切り抜きとコピーの両方がこれを参照する。
    property bool selectionActive: false
    property int selectionX: 0
    property int selectionY: 0
    property int selectionW: 0
    property int selectionH: 0

    function text(ja, en) {
        return appController.english ? en : ja
    }

    function resetView() {
        zoom = 1.0
        panX = 0
        panY = 0
    }

    function clearSelection() {
        selectionActive = false
        selectionW = 0
        selectionH = 0
    }

    function clampPan() {
        const scale = viewport.displayScale
        const maxX = Math.max(0, (appController.imageWidth * scale - viewport.width) / 2)
        const maxY = Math.max(0, (appController.imageHeight * scale - viewport.height) / 2)
        panX = Math.max(-maxX, Math.min(maxX, panX))
        panY = Math.max(-maxY, Math.min(maxY, panY))
    }

    // Win32版と同じく、カーソル位置を固定点として1段あたり1.15倍で拡大縮小する。
    function zoomAt(steps, cursorX, cursorY) {
        if (!appController.hasImage) return
        const oldZoom = zoom
        const newZoom = Math.max(0.1, Math.min(20.0, zoom * Math.pow(1.15, steps)))
        if (newZoom === oldZoom) return
        const oldScale = viewport.fitScale * oldZoom
        const newScale = viewport.fitScale * newZoom
        if (oldScale <= 0 || newScale <= 0) return
        const oldOriginX = (viewport.width - appController.imageWidth * oldScale) / 2 + panX
        const oldOriginY = (viewport.height - appController.imageHeight * oldScale) / 2 + panY
        const imageX = (cursorX - oldOriginX) / oldScale
        const imageY = (cursorY - oldOriginY) / oldScale
        zoom = newZoom
        panX = (cursorX - imageX * newScale) - (viewport.width - appController.imageWidth * newScale) / 2
        panY = (cursorY - imageY * newScale) - (viewport.height - appController.imageHeight * newScale) / 2
        clampPan()
    }

    function setSelectionFromView(x1, y1, x2, y2) {
        const scale = viewport.displayScale
        if (appController.imageWidth <= 0 || scale <= 0) return
        const left = Math.min(x1, x2)
        const right = Math.max(x1, x2)
        const top = Math.min(y1, y2)
        const bottom = Math.max(y1, y2)
        const ix1 = Math.round(Math.max(0, (left - viewport.originX) / scale))
        const iy1 = Math.round(Math.max(0, (top - viewport.originY) / scale))
        const ix2 = Math.round(Math.min(appController.imageWidth, (right - viewport.originX) / scale))
        const iy2 = Math.round(Math.min(appController.imageHeight, (bottom - viewport.originY) / scale))
        selectionX = ix1
        selectionY = iy1
        selectionW = Math.max(0, ix2 - ix1)
        selectionH = Math.max(0, iy2 - iy1)
        selectionActive = selectionW > 0 && selectionH > 0
    }

    // リサイズ後もウィンドウにフィットさせ直すと見た目が変わらないため、
    // Win32版のResizeCurrentImageForDisplayと同じく表示上の縮尺を維持する。
    function requestResize(widthValue, heightValue, percent, keepAspectRatio) {
        const previousDisplayScale = viewport.displayScale
        appController.resizeImageBy(widthValue, heightValue, percent, keepAspectRatio)
        const newFitScale = viewport.fitScale
        if (previousDisplayScale > 0 && newFitScale > 0) {
            zoom = Math.max(0.1, Math.min(20.0, previousDisplayScale / newFitScale))
        }
        clampPan()
    }

    function cropSelection() {
        if (!selectionActive) return
        appController.cropImage(selectionX, selectionY, selectionW, selectionH)
        clearSelection()
    }

    function copyCurrent() {
        if (!appController.hasImage) return
        if (selectionActive) appController.copyImageRegion(selectionX, selectionY, selectionW, selectionH)
        else appController.copyImage()
    }

    Connections {
        target: appController
        function onImageChanged() { window.clearSelection() }
        function onImageOpened() { window.resetView() }
    }

    component DarkMenuItem: MenuItem {
        id: darkMenuItem
        implicitWidth: 260
        implicitHeight: 30
        contentItem: Text {
            leftPadding: 10
            rightPadding: 10
            text: darkMenuItem.text
            font: darkMenuItem.font
            color: !darkMenuItem.enabled ? "#787d80" : "#e7edf5"
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: 4
            color: darkMenuItem.highlighted ? "#19353c" : "#121618"
        }
    }

    component DarkMenuBarItem: MenuBarItem {
        id: darkMenuBarItem
        implicitHeight: 30
        contentItem: Text {
            leftPadding: 12
            rightPadding: 12
            text: darkMenuBarItem.text
            font: darkMenuBarItem.font
            color: "#e7edf5"
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            color: (darkMenuBarItem.highlighted || darkMenuBarItem.hovered) ? "#19353c" : "#0a0c10"
        }
    }

    component DarkMenuBackground: Rectangle {
        implicitWidth: 260
        implicitHeight: 40
        color: "#121618"
        border.color: "#373f42"
        radius: 5
    }

    palette.window: backgroundColor
    palette.windowText: textColor
    palette.button: raisedSurfaceColor
    palette.buttonText: textColor
    palette.base: surfaceColor
    palette.text: textColor
    palette.highlight: accentDarkColor
    palette.highlightedText: "#ffffff"

    menuBar: Rectangle {
        implicitHeight: 32
        color: "#0a0c10"
        border.color: window.borderColor

        RowLayout {
            anchors.fill: parent
            anchors.rightMargin: 6
            spacing: 0

            MenuBar {
                objectName: "mainMenuBar"
                Layout.fillWidth: true
                Layout.fillHeight: true
                delegate: DarkMenuBarItem {}
                background: Rectangle {
                    implicitHeight: 30
                    color: "transparent"
                }

        Menu {
            objectName: "fileMenu"
            title: window.text("ファイル", "File")
            background: DarkMenuBackground {}
            DarkMenuItem {
                objectName: "openImageMenuItem"
                Accessible.name: window.text("画像を開く", "Open image")
                text: window.text("画像を開く...", "Open image...") + "  Ctrl+O"
                onTriggered: imageFileDialog.open()
            }
            DarkMenuItem {
                objectName: "saveImageMenuItem"
                Accessible.name: window.text("別形式で保存", "Save as")
                text: window.text("別形式で保存...", "Save as...")
                enabled: appController.hasImage
                onTriggered: saveOptionsDialog.open()
            }
            DarkMenuItem {
                objectName: "closeImageMenuItem"
                Accessible.name: window.text("画像を閉じる", "Close image")
                text: window.text("画像を閉じる", "Close image")
                enabled: appController.hasImage
                onTriggered: appController.clearImage()
            }
            MenuSeparator {}
            DarkMenuItem {
                objectName: "exitMenuItem"
                Accessible.name: window.text("終了", "Exit")
                text: window.text("終了", "Exit") + "  Ctrl+Q"
                onTriggered: window.close()
            }
        }

        Menu {
            objectName: "editMenu"
            title: window.text("編集", "Edit")
            background: DarkMenuBackground {}
            DarkMenuItem {
                objectName: "undoMenuItem"
                Accessible.name: window.text("元に戻す", "Undo")
                text: window.text("元に戻す", "Undo") + "  Ctrl+Z"
                enabled: appController.canUndo
                onTriggered: appController.undo()
            }
            DarkMenuItem {
                objectName: "redoMenuItem"
                Accessible.name: window.text("やり直す", "Redo")
                text: window.text("やり直す", "Redo") + "  Ctrl+Y"
                enabled: appController.canRedo
                onTriggered: appController.redo()
            }
        }

        Menu {
            objectName: "helpMenu"
            title: window.text("ヘルプ", "Help")
            background: DarkMenuBackground {}
            DarkMenuItem {
                objectName: "helpMenuItem"
                Accessible.name: window.text("ヘルプ", "Help")
                text: window.text("ヘルプ", "Help")
                onTriggered: helpDialog.open()
            }
            DarkMenuItem {
                objectName: "aboutMenuItem"
                Accessible.name: window.text("バージョン情報", "About")
                text: window.text("バージョン情報", "About")
                onTriggered: aboutDialog.open()
            }
            DarkMenuItem {
                objectName: "exifMenuItem"
                Accessible.name: window.text("ファイル情報 / EXIF情報", "File and EXIF information")
                text: infoWindow.visible ? window.text("情報ウィンドウを隠す", "Hide information window")
                                         : window.text("情報ウィンドウを表示", "Show information window")
                onTriggered: infoWindow.visible = !infoWindow.visible
            }
        }
            }

            Button {
                id: languageButton
                objectName: "languageToggleButton"
                Layout.alignment: Qt.AlignVCenter
                Accessible.name: window.text("Englishに切り替え", "Switch to Japanese")
                text: appController.english ? "🌐 日本語" : "🌐 English"
                ToolTip.visible: hovered
                ToolTip.text: "Toggle Language / 言語切替"
                onClicked: appController.setEnglish(!appController.english)
                contentItem: Text {
                    text: languageButton.text
                    color: languageButton.hovered ? "#ffffff" : window.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }
                background: Rectangle {
                    implicitWidth: 108
                    implicitHeight: 26
                    radius: 6
                    color: languageButton.hovered ? "#1f3d48" : "#161b22"
                    border.color: languageButton.hovered ? window.accentColor : window.borderColor
                }
            }
        }
    }

    FileDialog {
        id: imageFileDialog
        objectName: "imageFileDialog"
        title: window.text("画像を開く", "Open image")
        nameFilters: [window.text("画像ファイル (*.jpg *.jpeg *.png *.tif *.tiff *.bmp *.gif *.webp *.heic *.heif)",
                                  "Image files (*.jpg *.jpeg *.png *.tif *.tiff *.bmp *.gif *.webp *.heic *.heif)")]
        onAccepted: appController.openImage(selectedFile)
    }

    Dialog {
        id: saveOptionsDialog
        objectName: "saveOptionsDialog"
        modal: true
        title: window.text("保存オプション", "Save options")
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Ok | Dialog.Cancel
        contentItem: GridLayout {
            columns: 2
            rowSpacing: 10
            columnSpacing: 14
            Label { text: window.text("JPEG/WebP品質 (0-100)", "JPEG/WebP quality (0-100)"); color: window.textColor }
            SpinBox { id: saveQuality; objectName: "saveQualitySpinBox"; from: 0; to: 100; value: 90; editable: true }
            Label { text: window.text("PNG/TIFF圧縮 (0-9)", "PNG/TIFF compression (0-9)"); color: window.textColor }
            SpinBox { id: saveCompression; objectName: "saveCompressionSpinBox"; from: 0; to: 9; value: 6; editable: true }
        }
        onAccepted: {
            appController.setSaveOptions(saveQuality.value, saveCompression.value)
            saveImageDialog.open()
        }
    }

    FileDialog {
        id: saveImageDialog
        objectName: "saveImageDialog"
        fileMode: FileDialog.SaveFile
        title: window.text("別形式で保存", "Save as")
        nameFilters: ["PNG (*.png)", "JPEG (*.jpg *.jpeg)", "BMP (*.bmp)", "TIFF (*.tif *.tiff)", "WebP (*.webp)", "HEIC/HEIF (*.heic *.heif)"]
        onAccepted: appController.saveImage(selectedFile)
    }

    // 幅・高さの初期値100は「パーセント」を意味する。Win32版のリサイズダイアログと同じ既定値。
    // 縦横比ロック中は、変更した側に合わせて他方の表示値も更新する。
    Dialog {
        id: resizeDialog
        objectName: "resizeDialog"
        modal: true
        title: window.text("リサイズを指定", "Custom resize")
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Ok | Dialog.Cancel
        property bool resizeUpdating: false
        property int lastResizeMode: 0

        function applyAspectFromWidth() {
            if (resizeUpdating || !resizeLock.checked) return
            resizeUpdating = true
            if (resizeMode.currentIndex === 0) {
                resizeHeight.value = resizeWidth.value
            } else {
                const srcW = Math.max(1, appController.imageWidth)
                const srcH = Math.max(1, appController.imageHeight)
                resizeHeight.value = Math.max(1, Math.round(resizeWidth.value * srcH / srcW))
            }
            resizeUpdating = false
        }

        function applyAspectFromHeight() {
            if (resizeUpdating || !resizeLock.checked) return
            resizeUpdating = true
            if (resizeMode.currentIndex === 0) {
                resizeWidth.value = resizeHeight.value
            } else {
                const srcW = Math.max(1, appController.imageWidth)
                const srcH = Math.max(1, appController.imageHeight)
                resizeWidth.value = Math.max(1, Math.round(resizeHeight.value * srcW / srcH))
            }
            resizeUpdating = false
        }

        function convertResizeMode(previousIndex, nextIndex) {
            if (previousIndex === nextIndex) return
            const srcW = Math.max(1, appController.imageWidth)
            const srcH = Math.max(1, appController.imageHeight)
            resizeUpdating = true
            if (previousIndex === 0 && nextIndex === 1) {
                const widthPx = Math.max(1, Math.round(resizeWidth.value * srcW / 100.0))
                resizeWidth.value = widthPx
                resizeHeight.value = resizeLock.checked
                    ? Math.max(1, Math.round(widthPx * srcH / srcW))
                    : Math.max(1, Math.round(resizeHeight.value * srcH / 100.0))
            } else if (previousIndex === 1 && nextIndex === 0) {
                const widthPct = Math.max(1, Math.round(resizeWidth.value * 100.0 / srcW))
                resizeWidth.value = widthPct
                resizeHeight.value = resizeLock.checked
                    ? widthPct
                    : Math.max(1, Math.round(resizeHeight.value * 100.0 / srcH))
            }
            resizeUpdating = false
        }

        onAboutToShow: {
            resizeUpdating = true
            resizeWidth.value = 100
            resizeHeight.value = 100
            resizeMode.currentIndex = 0
            lastResizeMode = 0
            resizeLock.checked = true
            resizeUpdating = false
        }
        contentItem: GridLayout {
            columns: 2
            rowSpacing: 10
            columnSpacing: 14
            Label { text: window.text("幅", "Width"); color: window.textColor }
            SpinBox {
                id: resizeWidth
                objectName: "resizeWidthSpinBox"
                from: 1
                to: 100000
                value: 100
                editable: true
                onValueModified: resizeDialog.applyAspectFromWidth()
            }
            Label { text: window.text("高さ", "Height"); color: window.textColor }
            SpinBox {
                id: resizeHeight
                objectName: "resizeHeightSpinBox"
                from: 1
                to: 100000
                value: 100
                editable: true
                onValueModified: resizeDialog.applyAspectFromHeight()
            }
            Label { text: window.text("単位", "Mode"); color: window.textColor }
            ComboBox {
                id: resizeMode
                objectName: "resizeModeComboBox"
                model: [window.text("パーセント", "Percent"), window.text("ピクセル", "Pixels")]
                onActivated: {
                    resizeDialog.convertResizeMode(resizeDialog.lastResizeMode, currentIndex)
                    resizeDialog.lastResizeMode = currentIndex
                }
            }
            Item { Layout.preferredWidth: 1; Layout.preferredHeight: 1 }
            CheckBox {
                id: resizeLock
                objectName: "resizeLockCheckBox"
                text: window.text("縦横比を保持", "Keep aspect ratio")
                checked: true
                onToggled: if (checked) resizeDialog.applyAspectFromWidth()
            }
        }
        onAccepted: window.requestResize(resizeWidth.value, resizeHeight.value,
                                         resizeMode.currentIndex === 0, resizeLock.checked)
    }

    Shortcut { sequence: "Ctrl+O"; onActivated: imageFileDialog.open() }
    Shortcut { sequence: "Ctrl+C"; enabled: appController.hasImage; onActivated: window.copyCurrent() }
    Shortcut { sequence: "Ctrl+V"; onActivated: appController.pasteImage() }
    Shortcut { sequence: "Ctrl+Z"; enabled: appController.canUndo; onActivated: appController.undo() }
    Shortcut { sequence: "Ctrl+Y"; enabled: appController.canRedo; onActivated: appController.redo() }
    Shortcut { sequence: "Ctrl+Q"; onActivated: window.close() }

    Menu {
        id: imageContextMenu
        objectName: "imageContextMenu"
        background: DarkMenuBackground {}

        DarkMenuItem {
            objectName: "contextOpenImageMenuItem"
            Accessible.name: window.text("画像を開く", "Open image")
            text: window.text("画像を開く...", "Open image...")
            onTriggered: imageFileDialog.open()
        }
        DarkMenuItem {
            objectName: "contextSaveImageMenuItem"
            text: window.text("別形式で保存...", "Save as...")
            enabled: appController.hasImage
            onTriggered: saveOptionsDialog.open()
        }
        MenuSeparator {}
        DarkMenuItem {
            objectName: "contextResizeMenuItem"
            Accessible.name: window.text("リサイズを指定", "Custom resize")
            text: window.text("リサイズを指定...", "Custom resize...")
            enabled: appController.hasImage
            onTriggered: resizeDialog.open()
        }
        DarkMenuItem {
            objectName: "cropMenuItem"
            Accessible.name: window.text("選択範囲を切り抜く", "Crop selection")
            text: window.text("選択範囲を切り抜く", "Crop selection")
            enabled: appController.hasImage && window.selectionActive
            onTriggered: window.cropSelection()
        }
        MenuSeparator {}
        DarkMenuItem {
            objectName: "rotateRightMenuItem"
            text: window.text("右へ90度回転", "Rotate right 90 degrees")
            enabled: appController.hasImage
            onTriggered: appController.rotateRight()
        }
        DarkMenuItem {
            objectName: "rotate180MenuItem"
            text: window.text("180度回転", "Rotate 180 degrees")
            enabled: appController.hasImage
            onTriggered: appController.rotate180()
        }
        DarkMenuItem {
            objectName: "rotateLeftMenuItem"
            text: window.text("左へ90度回転", "Rotate left 90 degrees")
            enabled: appController.hasImage
            onTriggered: appController.rotateLeft()
        }
        DarkMenuItem {
            objectName: "flipHorizontalMenuItem"
            text: window.text("左右反転", "Flip horizontally")
            enabled: appController.hasImage
            onTriggered: appController.flipHorizontal()
        }
        DarkMenuItem {
            objectName: "flipVerticalMenuItem"
            text: window.text("上下反転", "Flip vertically")
            enabled: appController.hasImage
            onTriggered: appController.flipVertical()
        }
        Menu {
            objectName: "colorModeMenu"
            title: window.text("色変換", "Color mode")
            background: DarkMenuBackground {}
            DarkMenuItem {
                objectName: "colorFullMenuItem"
                text: window.text("フルカラー", "Full color")
                enabled: appController.hasImage
                onTriggered: appController.convertToFullColor()
            }
            DarkMenuItem {
                objectName: "color256MenuItem"
                text: window.text("256色", "256 colors")
                enabled: appController.hasImage
                onTriggered: appController.convertTo256Colors()
            }
            DarkMenuItem {
                objectName: "colorGrayMenuItem"
                text: window.text("グレースケール", "Grayscale")
                enabled: appController.hasImage
                onTriggered: appController.convertToGrayscale()
            }
        }
        MenuSeparator {}
        DarkMenuItem {
            objectName: "copyImageMenuItem"
            Accessible.name: window.text("画像をコピー", "Copy image")
            text: window.selectionActive ? window.text("選択範囲をコピー", "Copy selection")
                                         : window.text("画像をコピー", "Copy image")
            enabled: appController.hasImage
            onTriggered: window.copyCurrent()
        }
        DarkMenuItem {
            objectName: "pasteImageMenuItem"
            Accessible.name: window.text("画像を貼り付け", "Paste image")
            text: window.text("画像を貼り付け", "Paste image")
            onTriggered: appController.pasteImage()
        }
        MenuSeparator { visible: appController.pastePending }
        DarkMenuItem {
            objectName: "commitPasteMenuItem"
            visible: appController.pastePending
            height: visible ? implicitHeight : 0
            text: window.text("貼り付けを確定", "Commit paste")
            onTriggered: appController.commitPaste()
        }
        DarkMenuItem {
            objectName: "retryPasteMenuItem"
            visible: appController.pastePending
            height: visible ? implicitHeight : 0
            text: window.text("貼り付けをやり直す", "Retry paste")
            onTriggered: appController.retryPaste()
        }
    }

    Pane {
        anchors.fill: parent
        anchors.margins: 12
        objectName: "imageCanvasPanel"
        Accessible.name: window.text("画像表示領域", "Image view")
        padding: 1
        background: Rectangle { color: "#080d14"; radius: 12; border.color: window.borderColor }

        Item {
            anchors.fill: parent

            Item {
                id: viewport
                anchors.fill: parent
                anchors.margins: 18
                clip: true

                readonly property real fitScale:
                    (appController.imageWidth > 0 && appController.imageHeight > 0 && width > 0 && height > 0)
                        ? Math.min(width / appController.imageWidth, height / appController.imageHeight)
                        : 1.0
                readonly property real displayScale: fitScale * window.zoom
                readonly property real displayWidth: appController.imageWidth * displayScale
                readonly property real displayHeight: appController.imageHeight * displayScale
                readonly property real originX: (width - displayWidth) / 2 + window.panX
                readonly property real originY: (height - displayHeight) / 2 + window.panY

                Image {
                    id: imageView
                    objectName: "imageView"
                    Accessible.name: window.text("表示中の画像", "Displayed image")
                    x: viewport.originX
                    y: viewport.originY
                    width: viewport.displayWidth
                    height: viewport.displayHeight
                    source: appController.imageSource
                    fillMode: Image.Stretch
                    smooth: true
                    mipmap: true
                    asynchronous: true
                    visible: appController.hasImage
                }

                Rectangle {
                    id: selectionRect
                    objectName: "cropSelectionRectangle"
                    visible: window.selectionActive
                    x: viewport.originX + window.selectionX * viewport.displayScale
                    y: viewport.originY + window.selectionY * viewport.displayScale
                    width: window.selectionW * viewport.displayScale
                    height: window.selectionH * viewport.displayScale
                    color: Qt.rgba(0, 201, 232, 0.18)
                    border.color: window.accentColor
                    border.width: 2
                    z: 3
                }

                MouseArea {
                    id: imageMouseArea
                    objectName: "imageMouseArea"
                    Accessible.name: window.text("画像操作領域", "Image interaction area")
                    anchors.fill: parent
                    z: 4
                    acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton

                    property bool selecting: false
                    property bool panning: false
                    property real pressViewX: 0
                    property real pressViewY: 0
                    property real lastPanPointX: 0
                    property real lastPanPointY: 0

                    onPressed: mouse => {
                        if (mouse.button === Qt.MiddleButton) {
                            if (!appController.hasImage) return
                            panning = true
                            lastPanPointX = mouse.x
                            lastPanPointY = mouse.y
                        } else if (mouse.button === Qt.LeftButton) {
                            if (!appController.hasImage || appController.pastePending) return
                            selecting = true
                            pressViewX = mouse.x
                            pressViewY = mouse.y
                            window.setSelectionFromView(pressViewX, pressViewY, mouse.x, mouse.y)
                        }
                    }
                    onPositionChanged: mouse => {
                        if (panning) {
                            window.panX += mouse.x - lastPanPointX
                            window.panY += mouse.y - lastPanPointY
                            lastPanPointX = mouse.x
                            lastPanPointY = mouse.y
                            window.clampPan()
                        } else if (selecting) {
                            window.setSelectionFromView(pressViewX, pressViewY, mouse.x, mouse.y)
                        }
                    }
                    onReleased: mouse => {
                        if (mouse.button === Qt.MiddleButton) {
                            panning = false
                            return
                        }
                        if (mouse.button !== Qt.LeftButton || !selecting) return
                        selecting = false
                        window.setSelectionFromView(pressViewX, pressViewY, mouse.x, mouse.y)
                        // Win32版と同じく、4ピクセル以下のドラッグは選択解除として扱う。
                        if (Math.abs(mouse.x - pressViewX) <= 4 || Math.abs(mouse.y - pressViewY) <= 4) {
                            window.clearSelection()
                        }
                    }
                    onCanceled: {
                        selecting = false
                        panning = false
                    }
                    onClicked: mouse => {
                        if (mouse.button === Qt.RightButton) imageContextMenu.popup()
                    }
                    onWheel: wheel => {
                        if (!appController.hasImage) return
                        window.zoomAt(wheel.angleDelta.y / 120, wheel.x, wheel.y)
                    }
                }

                Image {
                    id: pasteView
                    objectName: "pasteImageView"
                    Accessible.name: window.text("移動中の貼り付け画像", "Pasted image being positioned")
                    visible: appController.pastePending
                    source: appController.pasteSource
                    x: viewport.originX + appController.pasteX * viewport.displayScale
                    y: viewport.originY + appController.pasteY * viewport.displayScale
                    width: appController.pasteWidth * viewport.displayScale
                    height: appController.pasteHeight * viewport.displayScale
                    fillMode: Image.Stretch
                    asynchronous: true
                    opacity: 0.85
                    z: 5

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        // このMouseAreaは移動する画像の子なので、ローカル座標のままだと
                        // 画像が動くたびに基準がずれて追従が揺れる。静止しているviewport座標へ変換して扱う。
                        property real dragStartX: 0
                        property real dragStartY: 0
                        property int pasteStartX: 0
                        property int pasteStartY: 0
                        onPressed: mouse => {
                            const origin = mapToItem(viewport, mouse.x, mouse.y)
                            dragStartX = origin.x
                            dragStartY = origin.y
                            pasteStartX = appController.pasteX
                            pasteStartY = appController.pasteY
                        }
                        onPositionChanged: mouse => {
                            if (!pressed || viewport.displayScale <= 0) return
                            const current = mapToItem(viewport, mouse.x, mouse.y)
                            appController.movePaste(
                                Math.round(pasteStartX + (current.x - dragStartX) / viewport.displayScale),
                                Math.round(pasteStartY + (current.y - dragStartY) / viewport.displayScale))
                        }
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: !appController.hasImage
                text: window.text("画像を開くと、ここに表示されます。", "The selected image is shown here.")
                color: window.mutedTextColor
                font.pixelSize: 18
            }

            Label {
                objectName: "statusLabel"
                Accessible.name: window.text("状態メッセージ", "Status message")
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.margins: 16
                text: appController.statusText
                color: window.mutedTextColor
                visible: text.length > 0
            }

            DropArea {
                objectName: "imageDropArea"
                Accessible.name: window.text("画像ファイルのドロップ領域", "Image file drop area")
                anchors.fill: parent
                onDropped: drop => {
                    if (drop.hasUrls && drop.urls.length > 0) appController.openImage(drop.urls[0])
                }
            }
        }
    }

    AboutDialog {
        id: aboutDialog
        english: appController.english
        appVersion: appController.appVersion
    }
    HelpView {
        id: helpDialog
        english: appController.english
        helpText: appController.helpText
    }
    InfoWindow {
        id: infoWindow
        host: window
        visible: true
        english: appController.english
        fileInfoText: appController.fileInfoText
        exifText: appController.exifText
        surfaceColor: window.surfaceColor
        borderColor: window.borderColor
        textColor: window.textColor
        mutedTextColor: window.mutedTextColor
        raisedSurfaceColor: window.raisedSurfaceColor
        onCopyExifRequested: appController.copyExif()
    }
}
