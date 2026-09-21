import QtQuick
import QtQuick.Controls
import QtTest
import "../../src/ui" as QuickImageView

// The window opens at 720x480 and can be shrunk to 480x320. Every dialog must stay inside the window
// (the Help dialog used to be 760x620 and the About dialog 380x510, which no longer fit).
TestCase {
    id: testCase
    name: "DialogsFitTheWindow"

    property string longText: "# Help\n\n" + "Some help text that is long enough to need scrolling in a small window.\n\n".repeat(60)

    ApplicationWindow {
        id: host
        width: 720
        height: 480
        visible: true

        QuickImageView.AboutDialog { id: aboutDialog; english: false; appVersion: "9.9.9" }
        QuickImageView.HelpView { id: helpDialog; english: false; helpText: testCase.longText }
        QuickImageView.ReplaceImageDialog { id: replaceDialog; english: false }
        QuickImageView.SettingsDialog { id: settingsDialog; english: false }
    }

    function resizeHost(w, h) {
        host.width = w
        host.height = h
        tryVerify(function() { return host.width === w && host.height === h }, 3000)
        // A window resize reaches the overlay asynchronously; the dialogs measure against the overlay.
        tryVerify(function() {
            return aboutDialog.maxDialogWidth === w - 32 && aboutDialog.maxDialogHeight === h - 32
        }, 5000)
    }

    function openAndCheck(dialog, label) {
        tryVerify(function() { return host.visible }, 3000)
        dialog.open()
        tryVerify(function() { return dialog.opened }, 3000)
        verify(dialog.width <= host.width, label + " is wider than the window: " + dialog.width + " > " + host.width)
        verify(dialog.height <= host.height, label + " is taller than the window: " + dialog.height + " > " + host.height)
        verify(dialog.x >= 0 && dialog.y >= 0, label + " starts outside the window: " + dialog.x + "," + dialog.y)
        verify(dialog.x + dialog.width <= host.width, label + " ends outside the window (right)")
        verify(dialog.y + dialog.height <= host.height, label + " ends outside the window (bottom)")
        dialog.close()
        tryVerify(function() { return !dialog.visible }, 3000)
    }

    function test_every_dialog_fits_the_smallest_window() {
        resizeHost(480, 320)
        openAndCheck(aboutDialog, "About")
        openAndCheck(helpDialog, "Help")
        openAndCheck(replaceDialog, "Replace confirmation")
        openAndCheck(settingsDialog, "Settings")
    }

    function test_every_dialog_fits_the_default_window() {
        resizeHost(720, 480)
        openAndCheck(aboutDialog, "About")
        openAndCheck(helpDialog, "Help")
        openAndCheck(replaceDialog, "Replace confirmation")
        openAndCheck(settingsDialog, "Settings")
    }

    function test_dialogs_keep_their_normal_size_in_a_large_window() {
        resizeHost(1400, 900)
        openAndCheck(aboutDialog, "About")
        compare(aboutDialog.width, 380)
        compare(aboutDialog.height, 510)
        openAndCheck(helpDialog, "Help")
        compare(helpDialog.width, 760)
        compare(helpDialog.height, 620)
    }

    function test_settings_shows_the_saved_window_size_and_its_limits() {
        resizeHost(720, 480)
        settingsDialog.open()
        tryVerify(function() { return settingsDialog.opened }, 3000)
        const widthBox = findChild(settingsDialog, "windowWidthSpinBox")
        const heightBox = findChild(settingsDialog, "windowHeightSpinBox")
        verify(widthBox !== null && heightBox !== null)
        compare(widthBox.value, 720)          // the default size
        compare(heightBox.value, 480)
        compare(widthBox.from, 480)           // the window's minimum
        compare(heightBox.from, 320)
        compare(widthBox.to, 7680)
        compare(heightBox.to, 4320)
        settingsDialog.close()
        tryVerify(function() { return !settingsDialog.visible }, 3000)
    }
}
