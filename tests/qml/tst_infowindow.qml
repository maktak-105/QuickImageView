import QtQuick
import QtQuick.Window
import QtTest
import "../../src/ui" as QuickImageView

// 情報ウィンドウの配置は本体側の値だけで決まること。
// 自分自身のScreenやwidthに依存すると、DPIの異なるモニター間で配置が往復してハングする（v4.0.0で発生）。
TestCase {
    name: "InfoWindowPlacement"

    Window {
        id: hostWindow
        x: 100
        y: 50
        width: 1180
        height: 760
    }

    Component {
        id: infoComponent
        QuickImageView.InfoWindow {
            host: hostWindow
            hostWorkAreaLeft: 0
            hostWorkAreaTop: 0
            hostWorkAreaRight: 3000
        }
    }

    function init() {
        hostWindow.x = 100
        hostWindow.y = 50
    }

    function test_places_right_of_host_when_it_fits() {
        const info = createTemporaryObject(infoComponent, null)
        verify(info !== null)
        compare(info.x, 100 + 1180 + info.gap)
        compare(info.y, 50 + 40)
    }

    function test_falls_back_to_left_when_it_does_not_fit() {
        const info = createTemporaryObject(infoComponent, null, {hostWorkAreaRight: 1500})
        hostWindow.x = 600
        compare(info.x, 600 - info.preferredWidth - info.gap)
    }

    function test_left_fallback_is_clamped_to_the_work_area() {
        const info = createTemporaryObject(infoComponent, null, {hostWorkAreaRight: 1500, hostWorkAreaLeft: 20})
        compare(info.x, 20)
    }

    function test_placement_does_not_depend_on_the_info_window_size() {
        const info = createTemporaryObject(infoComponent, null)
        const before = info.x
        info.width = 700
        compare(info.x, before)
        info.width = 320
        compare(info.x, before)
    }

    function test_follows_the_host() {
        const info = createTemporaryObject(infoComponent, null)
        hostWindow.x = 300
        hostWindow.y = 200
        compare(info.x, 300 + 1180 + info.gap)
        compare(info.y, 200 + 40)
    }
}
