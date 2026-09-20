from pywinauto import Desktop
for window in Desktop(backend="win32").windows():
    try:
        if window.is_visible():
            print(window.handle, repr(window.window_text()), window.class_name())
    except Exception:
        pass
