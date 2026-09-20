from pathlib import Path
import sys
import time

from pywinauto import Application
import win32gui

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tests"))
from run_loop import make_exif_jpeg

root = Path(__file__).resolve().parents[2]
image = root / "assets" / "qiv-screenshot-input.jpg"
make_exif_jpeg(image)
app = Application(backend="win32").start(f'"{root / "build" / "QuickImageView.exe"}" "{image}"')
window = app.window(class_name="QuickImageViewWindow")
window.wait("visible", timeout=8)
time.sleep(1)
window.capture_as_image().save(root / "assets" / "QuickImageView-gui-ja.png")
win32gui.SendMessage(window.handle, 0x0111, 1200, 0)
time.sleep(1)
window.capture_as_image().save(root / "assets" / "QuickImageView-gui-en.png")
app.kill()
image.unlink(missing_ok=True)
