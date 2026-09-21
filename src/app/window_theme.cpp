#include "window_theme.h"

#include <QEvent>
#include <QWindow>
#include <QtGui/qevent.h>

#include <windows.h>

namespace {

class DarkTitleBarFilter final : public QObject {
public:
    using QObject::QObject;

    bool eventFilter(QObject* watched, QEvent* event) override {
        if (event->type() == QEvent::PlatformSurface) {
            const auto* surfaceEvent = static_cast<QPlatformSurfaceEvent*>(event);
            if (surfaceEvent->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated) {
                WindowTheme::applyDarkTitleBar(qobject_cast<QWindow*>(watched));
            }
        }
        return false;
    }
};

}  // namespace

namespace WindowTheme {

void applyDarkTitleBar(QWindow* window) {
    if (!window) return;
    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd) return;
    HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
    if (!dwm) return;
    using SetAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
    auto setAttribute = reinterpret_cast<SetAttributeFn>(GetProcAddress(dwm, "DwmSetWindowAttribute"));
    if (setAttribute) {
        const BOOL enabled = TRUE;
        // 20 = DWMWA_USE_IMMERSIVE_DARK_MODE (Windows 10 build 18985 and later), 19 on older builds.
        if (FAILED(setAttribute(hwnd, 20, &enabled, sizeof(enabled)))) {
            setAttribute(hwnd, 19, &enabled, sizeof(enabled));
        }
    }
    FreeLibrary(dwm);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

void installDarkTitleBar(QObject* application) {
    application->installEventFilter(new DarkTitleBarFilter(application));
}

}  // namespace WindowTheme
