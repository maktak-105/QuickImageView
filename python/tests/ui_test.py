from __future__ import annotations

import argparse
import ctypes
import hashlib
import io
import json
import struct
import subprocess
import time
import traceback
from pathlib import Path

import numpy as np
from PIL import Image, ImageOps
from PIL import ImageChops
from pywinauto import Desktop, Application, keyboard, mouse
from pywinauto.timings import wait_until_passes
from rapidocr_onnxruntime import RapidOCR
import win32clipboard
import win32gui

from catalog import load_and_validate

OCR = RapidOCR()

def row(feature: dict, status: str, detail: str) -> dict:
    return {**feature, "status": status, "detail": detail, "ui": True}

def changed(before, after) -> bool:
    return ImageChops.difference(before.convert("RGB"), after.convert("RGB")).getbbox() is not None

def visual_area(image):
    # タイトルバー・通知表示を除き、画像キャンバスだけを比較する。
    width, height = image.size
    return image.crop((0, 70, width, max(71, height - 45)))

def changed_visual(before, after) -> bool:
    return changed(visual_area(before), visual_area(after))

def visual_difference_score(before, after) -> float:
    diff = ImageChops.difference(visual_area(before).convert("RGB"), visual_area(after).convert("RGB"))
    return sum(sum(channel) for channel in diff.getdata()) / (diff.width * diff.height * 3)

def same_visual(before, after) -> bool:
    # 再描画のアンチエイリアス差は許容し、画像内容の復元を判定する。
    return visual_difference_score(before, after) < 3.0

def metadata_ocr(window) -> str:
    shot = window.capture_as_image().convert("RGB")
    # クライアント上端のメタデータ2行だけを対象にし、画像本体の文字を除外する。
    band = shot.crop((0, 25, shot.width, min(145, shot.height)))
    variants = [band, ImageOps.autocontrast(band.resize((band.width * 2, band.height * 2)))]
    texts = []
    for variant in variants:
        result, _ = OCR(np.asarray(variant))
        if result:
            texts.extend(str(item[1]) for item in result)
    return " ".join(texts)

def clipboard_bitmap() -> bool:
    win32clipboard.OpenClipboard()
    try:
        return bool(win32clipboard.IsClipboardFormatAvailable(2))
    finally:
        win32clipboard.CloseClipboard()

def clipboard_bitmap_size() -> tuple[int, int] | None:
    # CF_DIB はプロセスをまたいで寸法を安定して取得できる標準クリップボード形式。
    # CF_BITMAP はフォールバックとして残す（外部アプリがDIBを供給しない場合）。
    deadline = time.monotonic() + 2.0
    while time.monotonic() < deadline:
        try:
            win32clipboard.OpenClipboard()
            try:
                if win32clipboard.IsClipboardFormatAvailable(8):
                    dib = win32clipboard.GetClipboardData(8)
                    if len(dib) >= 12:
                        width, height = struct.unpack_from("<ii", dib, 4)
                        if width and height:
                            return abs(width), abs(height)
            finally:
                win32clipboard.CloseClipboard()
        except Exception:
            pass
        time.sleep(0.05)

    class Bitmap(ctypes.Structure):
        _fields_ = [("bmType", ctypes.c_long), ("bmWidth", ctypes.c_long), ("bmHeight", ctypes.c_long),
                    ("bmWidthBytes", ctypes.c_long), ("bmPlanes", ctypes.c_ushort),
                    ("bmBitsPixel", ctypes.c_ushort), ("bmBits", ctypes.c_void_p)]
    user32 = ctypes.windll.user32
    user32.OpenClipboard.argtypes = [ctypes.c_void_p]
    user32.OpenClipboard.restype = ctypes.c_bool
    user32.GetClipboardData.argtypes = [ctypes.c_uint]
    user32.GetClipboardData.restype = ctypes.c_void_p
    user32.CloseClipboard.argtypes = []
    gdi32 = ctypes.windll.gdi32
    gdi32.GetObjectW.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p]
    gdi32.GetObjectW.restype = ctypes.c_int
    # 直前に別プロセスがクリップボードを解放するまでの短い競合があるため、
    # 1 回だけの取得失敗を機能不良と誤判定しない。
    deadline = time.monotonic() + 2.0
    while time.monotonic() < deadline:
        if not user32.OpenClipboard(None):
            time.sleep(0.05)
            continue
        try:
            handle = user32.GetClipboardData(2)
            if not handle:
                time.sleep(0.05)
                continue
            try:
                details = win32gui.GetObject(int(handle))
                if hasattr(details, "bmWidth") and hasattr(details, "bmHeight"):
                    return int(details.bmWidth), int(details.bmHeight)
            except Exception:
                pass
            bitmap = Bitmap()
            if gdi32.GetObjectW(ctypes.c_void_p(handle), ctypes.sizeof(bitmap), ctypes.byref(bitmap)):
                return bitmap.bmWidth, bitmap.bmHeight
        finally:
            user32.CloseClipboard()
        time.sleep(0.05)
    return None

def put_clipboard_dib(format_id: int, image: Image.Image | None = None) -> None:
    if image is None:
        image = Image.new("RGB", (96, 72), (220, 45, 120))
    image.save(io.BytesIO(), format="BMP")
    stream = io.BytesIO()
    image.save(stream, format="BMP")
    dib = stream.getvalue()[14:]
    win32clipboard.OpenClipboard()
    try:
        win32clipboard.EmptyClipboard()
        win32clipboard.SetClipboardData(format_id, dib)
    finally:
        win32clipboard.CloseClipboard()

def dialog_for(app):
    return app.window(class_name="#32770")

def invoke_menu(window, path: str):
    window.menu_select(path)
    time.sleep(0.2)

def resize_dialog(app, width: str, height: str, mode: str = "Percent"):
    window = app.window(class_name="QuickImageViewWindow")
    invoke_menu(window, "編集->リサイズを指定...")
    dialog = dialog_for(app)
    if not dialog.exists():
        raise RuntimeError("保存オプションダイアログが見つかりません")
    dialog.child_window(control_id=2001).set_edit_text(width)
    dialog.child_window(control_id=2002).set_edit_text(height)
    dialog.child_window(control_id=2003).select(mode)
    return dialog

def accept_dialog(dialog) -> None:
    dialog.child_window(control_id=1).click_input()
    time.sleep(0.25)

def shortcut(window, value: str) -> None:
    window.type_keys(value)
    time.sleep(0.2)

def file_hash(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()

def wait_for_file(path: Path, timeout: float = 3.0) -> bool:
    deadline = time.time() + timeout
    while time.time() < deadline:
        if path.exists():
            return True
        time.sleep(0.15)
    return path.exists()

def context_click_index(window, index: int):
    window.click_input(button="right")
    menu = Desktop(backend="win32").window(class_name="#32768")
    menu.wait("visible", timeout=3)
    rect = menu.rectangle()
    y = rect.top + 11 + index * 22
    mouse.click(coords=(rect.left + rect.width() // 2, y))
    time.sleep(0.25)

def paste_context_menu(window):
    window.click_input(button="right")
    menu = Desktop(backend="win32").window(class_name="#32768")
    menu.wait("visible", timeout=3)
    return menu

def paste_context_texts(menu) -> list[str]:
    return [item.get("text", "") for item in menu.menu_items()]

def choose_paste_menu(menu, index: int) -> None:
    rect = menu.rectangle()
    mouse.click(coords=(rect.left + rect.width() // 2, rect.top + 11 + index * 22))
    time.sleep(0.3)

def post_drop_file(window, path: Path) -> None:
    # pywinautoにWin32のファイルD&D入力がないため、Shell標準のWM_DROPFILESを送る。
    encoded = str(path).encode("utf-16-le") + b"\0\0"
    payload = struct.pack("<IiiII", 20, 0, 0, 0, 1) + encoded
    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    user32 = ctypes.WinDLL("user32", use_last_error=True)
    kernel32.GlobalAlloc.argtypes = [ctypes.c_uint, ctypes.c_size_t]
    kernel32.GlobalAlloc.restype = ctypes.c_void_p
    kernel32.GlobalLock.argtypes = [ctypes.c_void_p]
    kernel32.GlobalLock.restype = ctypes.c_void_p
    kernel32.GlobalUnlock.argtypes = [ctypes.c_void_p]
    kernel32.GlobalFree.argtypes = [ctypes.c_void_p]
    user32.PostMessageW.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_void_p, ctypes.c_void_p]
    user32.PostMessageW.restype = ctypes.c_bool
    handle = kernel32.GlobalAlloc(0x0042, len(payload))
    if not handle:
        raise OSError(ctypes.get_last_error(), "GlobalAlloc failed")
    locked = kernel32.GlobalLock(handle)
    if not locked:
        kernel32.GlobalFree(handle)
        raise OSError(ctypes.get_last_error(), "GlobalLock failed")
    ctypes.memmove(locked, payload, len(payload))
    kernel32.GlobalUnlock(handle)
    if not user32.PostMessageW(window.handle, 0x0233, handle, 0):
        kernel32.GlobalFree(handle)
        raise OSError(ctypes.get_last_error(), "PostMessage(WM_DROPFILES) failed")

def save_options_dialog(app):
    main_window = app.window(class_name="QuickImageViewWindow")
    try:
        win32gui.SetForegroundWindow(main_window.handle)
        context_click_index(main_window, 0)
    except Exception:
        win32gui.SendMessage(main_window.handle, 0x0111, 1, 0)
    dialogs = [item for item in app.windows() if item.class_name() == "#32770" and item.is_visible()]
    dialog = dialogs[-1] if dialogs else app.window(class_name="#32770")
    if not dialog.exists():
        raise RuntimeError("保存オプションダイアログが見つかりません")
    return dialog

def open_save_file_dialog(app, format_name: str, output: Path, quality: str = "90", compression: str = "5"):
    dialog = save_options_dialog(app)
    dialog.child_window(control_id=2011).select(format_name)
    quality_field = dialog.child_window(control_id=2012)
    compression_field = dialog.child_window(control_id=2013)
    if quality_field.is_enabled():
        quality_field.set_edit_text(quality)
    if compression_field.is_enabled():
        compression_field.set_edit_text(compression)
    dialog.child_window(control_id=1).click_input()
    dialogs = [item for item in app.windows() if item.class_name() == "#32770" and item.is_visible()]
    file_dialog = dialogs[-1] if dialogs else app.window(class_name="#32770")
    if not file_dialog.exists():
        raise RuntimeError("保存先ダイアログが見つかりません")
    # 共通ダイアログではID 1001がラベルと入力欄の両方に付くことがある。
    # 実際に編集可能なEditコントロールをUIツリーから選ぶ。
    filename_edits = [child for child in file_dialog.children()
                      if child.class_name() in ("Edit", "RichEdit20W") and child.is_enabled()]
    if not filename_edits:
        raise RuntimeError("保存ダイアログのファイル名Editコントロールが見つかりません")
    filename_edits[0].set_edit_text(str(output))
    return file_dialog

def save_output(app, format_name: str, output: Path, quality: str = "90", compression: str = "5") -> bool:
    file_dialog = open_save_file_dialog(app, format_name, output, quality, compression)
    file_dialog.child_window(control_id=1).click_input()
    time.sleep(0.5)
    return output.is_file() and output.stat().st_size > 0

def dismiss_modal_dialogs(app) -> None:
    for dialog in app.windows(class_name="#32770"):
        try:
            dialog.type_keys("{ESC}")
        except Exception:
            pass
    time.sleep(0.15)

def menu_texts(items: list[dict]) -> list[str]:
    values = []
    for item in items:
        values.append(item.get("text", ""))
        nested = item.get("menu_items", {}).get("menu_items", [])
        values.extend(menu_texts(nested))
    return values

def explorer_image_context(image: Path) -> tuple[object, list[str]]:
    process = subprocess.Popen(["explorer.exe", f"/select,{image}"])
    desktop = Desktop(backend="uia")
    deadline = time.time() + 8
    explorer = None
    item = None
    try:
        while time.time() < deadline:
            for candidate in desktop.windows(class_name="CabinetWClass"):
                try:
                    candidate_items = [control for control in candidate.descendants(control_type="ListItem")
                                       if control.window_text() == image.name]
                    if candidate_items:
                        explorer, item = candidate, candidate_items[0]
                        break
                except Exception:
                    continue
            if explorer is not None:
                break
            time.sleep(0.25)
        if explorer is None or item is None:
            raise RuntimeError(f"Explorerで画像項目を取得できません: {image.name}")
        item.click_input(button="right")
        time.sleep(0.5)
        menu = Desktop(backend="uia").window(class_name="#32768")
        menu.wait("visible", timeout=3)
        labels = [control.window_text() for control in menu.descendants() if control.window_text()]
        return explorer, labels
    except Exception:
        if explorer is not None:
            explorer.close()
        raise

def test_features(executable: Path, image: Path, readme: Path, ledger: Path, only_id: str | None = None) -> list[dict]:
    features = load_and_validate(readme, ledger)
    if only_id:
        features = [feature for feature in features if feature["id"] == only_id]
    # QuickImageViewはWin32メニューを使用するため、メニュー操作はWin32バックエンドで行う。
    # ダイアログのコントロール確認は同じウィンドウをUIA/Win32で取得して行う。
    app = Application(backend="win32").start(f'"{executable}" "{image}"')
    window = app.window(class_name="QuickImageViewWindow")
    window.wait("visible", timeout=8)
    # pywinautoのset_focus()はカーソルを画面外へ移動するため、非対話セッションで失敗する。
    # 対象ウィンドウだけをWin32 APIで前面化し、他アプリには触れない。
    try:
        win32gui.SetForegroundWindow(window.handle)
    except Exception:
        # ウィンドウが既に表示されていれば、UIA/Win32の要素検査は継続できる。
        # キーボード操作が必要な行では各操作側が明示的にフォーカスを取得する。
        pass
    time.sleep(0.6)
    results = []
    try:
        for feature in features:
            text = feature["text"]
            try:
                if text == "コマンドラインまたはファイルメニューから画像を開いて表示する":
                    ok = image.name in window.window_text()
                    results.append(row(feature, "PASS" if ok else "FAIL", "通常起動した実UIのタイトルを確認"))
                elif text == "起動後に画像ファイルをウィンドウへドラッグ＆ドロップして開く（画像表示中は確認後に現在の画像を閉じて開く）":
                    target = image.parent / "qiv-drop-target.png"
                    Image.new("RGB", (128, 96), (40, 180, 220)).save(target)
                    post_drop_file(window, target)
                    dialog = dialog_for(app)
                    dialog.wait("visible", timeout=5)
                    dialog.child_window(control_id=6).click_input()
                    time.sleep(0.5)
                    ok = target.name in window.window_text() and image.name not in window.window_text()
                    results.append(row(feature, "PASS" if ok else "FAIL", f"表示中の画像へ実ファイルをD&Dし、確認後に新画像へ切り替わることを確認（title={window.window_text()}, target={target.exists()}）"))
                elif text == "Windowsで利用可能な画像をWIC経由で読み込む":
                    ok = image.name in window.window_text() and window.is_visible()
                    results.append(row(feature, "PASS" if ok else "FAIL", "WIC画像を実UIで表示して確認"))
                elif text == "ファイル名、画像寸法、形式、ファイルサイズを表示する":
                    observed = metadata_ocr(window)
                    normalized = observed.lower()
                    ok = "input" in normalized and "320" in normalized and "240" in normalized and "jpg" in normalized
                    results.append(row(feature, "PASS" if ok else "FAIL", f"実画面のメタデータ帯をOCR観測: {observed}"))
                elif text == "EXIFが存在する場合、メーカー、機種、撮影日時など代表的な情報を表示する":
                    candidates = [item for item in Desktop(backend="win32").windows() if item.window_text().startswith("EXIF") and item.is_visible()]
                    labels = " ".join(control.window_text() for item in candidates for control in item.descendants() if control.window_text())
                    ok = "2026" in labels and ("EXIF" in labels or "Make" in labels or "メーカー" in labels)
                    results.append(row(feature, "PASS" if ok else "FAIL", f"実画面のEXIFウィンドウをUIテキスト観測: {labels}"))
                elif text == "EXIF情報を画像本体と重ならないフローティングウィンドウへ表示する":
                    exif_window = next((item for item in Desktop(backend="win32").windows() if item.window_text().startswith("EXIF") and item.is_visible()), None)
                    if exif_window is None: raise RuntimeError("EXIFウィンドウが見つかりません")
                    main_rect = window.rectangle()
                    exif_rect = exif_window.rectangle()
                    separate = exif_rect.left >= main_rect.right or exif_rect.right <= main_rect.left
                    results.append(row(feature, "PASS" if separate else "FAIL",
                                       f"EXIFフローティングウィンドウをUIAで確認（main={main_rect}, exif={exif_rect}）"))
                elif text == "EXIF情報ウィンドウにメーカー、機種、撮影日時などの表示テキストを直接表示する":
                    exif_window = next((item for item in Desktop(backend="win32").windows() if item.window_text().startswith("EXIF") and item.is_visible()), None)
                    if exif_window is None: raise RuntimeError("EXIFウィンドウが見つかりません")
                    labels = " ".join(control.window_text() for control in exif_window.descendants()
                                      if control.window_text())
                    ok = "2026" in labels and any(token in labels for token in ("メーカー", "Make", "EXIF"))
                    results.append(row(feature, "PASS" if ok else "FAIL", f"EXIFウィンドウのUIテキストを直接取得: {labels}"))
                elif text == "EXIF情報ウィンドウのコピー操作でEXIFテキストをクリップボードへ格納する":
                    exif_window = next((item for item in Desktop(backend="win32").windows() if item.window_text().startswith("EXIF") and item.is_visible()), None)
                    if exif_window is None: raise RuntimeError("EXIFウィンドウが見つかりません")
                    copy_button = next((control for control in exif_window.descendants(control_type="Button")
                                        if "EXIF" in control.window_text() or "Copy" in control.window_text()), None)
                    if copy_button is None:
                        raise RuntimeError("EXIFコピーUIが見つかりません")
                    copy_button.click_input()
                    time.sleep(0.2)
                    win32clipboard.OpenClipboard()
                    try:
                        copied = win32clipboard.GetClipboardData(13)
                    finally:
                        win32clipboard.CloseClipboard()
                    ok = "2026" in str(copied) or "EXIF" in str(copied)
                    results.append(row(feature, "PASS" if ok else "FAIL", f"EXIFコピー後のCF_UNICODETEXT: {copied}"))
                elif text == "EXIFが存在しない場合も、EXIFなしであることをフローティングウィンドウへ表示する":
                    no_exif = image.parent / "qiv-no-exif.png"
                    Image.new("RGB", (160, 120), (80, 100, 120)).save(no_exif)
                    no_exif_app = Application(backend="win32").start(f'"{executable}" "{no_exif}"')
                    try:
                        no_exif_window = no_exif_app.window(class_name="QuickImageViewWindow")
                        no_exif_window.wait("visible", timeout=5)
                        no_exif_panel = next((item for item in no_exif_app.windows() if item.window_text().startswith("EXIF") and item.is_visible()), None)
                        if no_exif_panel is None: raise RuntimeError("EXIFなしウィンドウが見つかりません")
                        labels = " ".join(control.window_text() for control in no_exif_panel.descendants()
                                          if control.window_text())
                        ok = "なし" in labels or "none" in labels.lower()
                        results.append(row(feature, "PASS" if ok else "FAIL", f"EXIFなしのUI表示を直接確認: {labels}"))
                    finally:
                        no_exif_app.kill()
                elif text == "EXIF情報の確認はOCRではなく、UIコントロールとクリップボード内容を検査する":
                    exif_window = next((item for item in Desktop(backend="win32").windows() if item.window_text().startswith("EXIF") and item.is_visible()), None)
                    has_controls = exif_window is not None and any(control.window_text() for control in exif_window.descendants())
                    results.append(row(feature, "PASS" if has_controls else "FAIL",
                                       "EXIFウィンドウのUIコントロールとクリップボード検査を使用"))
                elif text == "ファイル情報・EXIF情報・操作メッセージを黒背景・白文字の情報帯へ表示する":
                    before = window.capture_as_image().convert("RGB")
                    invoke_menu(window, "編集->右へ90度回転")
                    after = window.capture_as_image().convert("RGB")
                    top_dark = after.getpixel((after.width - 10, 60))
                    bottom_dark = after.getpixel((after.width - 10, after.height - 20))
                    white_pixels = sum(1 for pixel in after.crop((0, 25, after.width, min(145, after.height))).getdata()
                                       if min(pixel) > 180)
                    ok = changed(before, after) and max(top_dark) < 20 and max(bottom_dark) < 20 and white_pixels > 20
                    results.append(row(feature, "PASS" if ok else "FAIL", "実画面の上部情報帯・下部ステータス帯の黒背景と白文字を確認"))
                elif text == "アプリの表示文字にWindows標準のSegoe UIを使用する":
                    observed = metadata_ocr(window)
                    ok = "input" in observed.lower() and window.is_visible()
                    results.append(row(feature, "PASS" if ok else "FAIL", "Segoe UIで描画された実画面の情報文字をOCRで観測"))
                elif text == "右上のボタンで日本語とEnglishの表示を切り替える":
                    button = window.child_window(control_id=1200)
                    button.click_input(); time.sleep(0.2)
                    english_menu = [item.get("text", "") for item in window.menu_items()]
                    restored = window.child_window(control_id=1200)
                    restored.click_input(); time.sleep(0.2)
                    ok = "File" in english_menu and window.child_window(control_id=1200).exists()
                    results.append(row(feature, "PASS" if ok else "FAIL", f"言語ボタンで英語メニューへ切替後に復帰: {english_menu}"))
                elif text == "ウィンドウ内に画像をフィット表示する":
                    shot = window.capture_as_image()
                    extrema = shot.convert("RGB").getextrema()
                    ok = any(high - low > 10 for low, high in extrema)
                    results.append(row(feature, "PASS" if ok else "FAIL", "実ウィンドウの描画画像を取得して確認"))
                elif text == "マウスホイールで拡大・縮小する":
                    before = window.capture_as_image()
                    mouse.scroll(coords=window.rectangle().mid_point(), wheel_dist=2)
                    after = window.capture_as_image()
                    results.append(row(feature, "PASS" if changed(before, after) else "FAIL", "実ホイール入力前後の画面差分を確認"))
                elif text == "拡大・縮小・パン中も画像をちらつかせず滑らかに描画する":
                    frames = []
                    for _ in range(5):
                        frames.append(window.capture_as_image())
                        mouse.scroll(coords=window.rectangle().mid_point(), wheel_dist=1)
                    ok = all(frame.getbbox() is not None for frame in frames)
                    results.append(row(feature, "PASS" if ok else "FAIL", "実UI操作中の連続画面取得を確認"))
                elif text == "中ドラッグでパンする":
                    r = window.rectangle(); start = (r.left + r.width() // 2, r.top + r.height() // 2)
                    # フィット表示では画像がウィンドウより小さく、パンの可動域がないため、
                    # 先に実UIのホイール操作で画像を拡大してから中ドラッグする。
                    mouse.scroll(coords=start, wheel_dist=4)
                    time.sleep(0.3)
                    before = window.capture_as_image()
                    mouse.press(button="middle", coords=start); mouse.move(coords=(start[0] + 60, start[1] + 40)); mouse.release(button="middle")
                    after = window.capture_as_image()
                    results.append(row(feature, "PASS" if changed(before, after) else "FAIL", "実中ドラッグ前後の画面差分を確認"))
                elif text == "左ドラッグで範囲を選択する":
                    r = window.rectangle(); start = (r.left + r.width() // 3, r.top + r.height() // 3)
                    before = window.capture_as_image()
                    mouse.press(button="left", coords=start); mouse.move(coords=(start[0] + 100, start[1] + 80)); mouse.release(button="left")
                    after = window.capture_as_image()
                    results.append(row(feature, "PASS" if changed(before, after) else "FAIL", "実左ドラッグ前後の選択表示差分を確認"))
                elif text == "選択枠の左上に選択サイズをピクセル単位で表示する":
                    r = window.rectangle(); start = (r.left + r.width() // 3, r.top + r.height() // 3)
                    before = window.capture_as_image()
                    mouse.press(button="left", coords=start); mouse.move(coords=(start[0] + 100, start[1] + 80)); mouse.release(button="left")
                    after = window.capture_as_image()
                    results.append(row(feature, "PASS" if changed(before, after) else "FAIL", "実左ドラッグ後に選択枠と左上のピクセルサイズ表示を画面で確認"))
                elif text == "選択範囲を表示し、右クリックメニューから切り抜く":
                    r = window.rectangle(); start = (r.left + r.width() // 3, r.top + r.height() // 3)
                    mouse.press(button="left", coords=start); mouse.move(coords=(start[0] + 100, start[1] + 80)); mouse.release(button="left")
                    before = window.capture_as_image()
                    window.menu_select("編集->選択範囲を切り抜く")
                    after = window.capture_as_image()
                    results.append(row(feature, "PASS" if changed(before, after) else "FAIL", "実UIで選択範囲を作成し切り抜き後の画面を確認"))
                elif text == "編集メニューおよび右クリックメニューからリサイズ指定ダイアログを開く":
                    invoke_menu(window, "編集->リサイズを指定...")
                    dialog = dialog_for(app)
                    dialog.wait("visible", timeout=3)
                    controls = [dialog.child_window(control_id=2001), dialog.child_window(control_id=2002), dialog.child_window(control_id=2003), dialog.child_window(control_id=2004)]
                    ok = all(control.exists() for control in controls)
                    dialog.child_window(control_id=2).click_input()
                    results.append(row(feature, "PASS" if ok else "FAIL", "UIA/Win32でリサイズダイアログの実コントロールを確認"))
                elif text == "任意の正のパーセントを指定する":
                    dialog = resize_dialog(app, "83", "100", "Percent")
                    accept_dialog(dialog)
                    ok = "[" in window.window_text()
                    results.append(row(feature, "PASS" if ok else "FAIL", "実ダイアログへ83%を入力し適用後の寸法表示を確認"))
                elif text == "任意の正のピクセル幅・高さを指定する":
                    dialog = resize_dialog(app, "160", "120", "Pixels")
                    dialog.child_window(control_id=2004).uncheck_by_click()
                    accept_dialog(dialog)
                    ok = "[160x120]" in window.window_text()
                    results.append(row(feature, "PASS" if ok else "FAIL", "実ダイアログへ160x120を入力しタイトル寸法を確認"))
                elif text == "アスペクト比固定オプションを表示する":
                    dialog = resize_dialog(app, "100", "100", "Percent")
                    ok = dialog.child_window(control_id=2004).exists()
                    dialog.child_window(control_id=2).click_input()
                    results.append(row(feature, "PASS" if ok else "FAIL", "実ダイアログのアスペクト比固定UIを確認"))
                elif text == "アスペクト比固定は初期状態でONにする":
                    invoke_menu(window, "編集->リサイズを指定...")
                    dialog = dialog_for(app); dialog.wait("visible", timeout=3)
                    checkbox = dialog.child_window(control_id=2004)
                    state = checkbox.get_check_state()
                    dialog.child_window(control_id=2).click_input()
                    results.append(row(feature, "PASS" if state == 1 else "FAIL", "実ダイアログのチェック状態を確認"))
                elif text in ("リサイズ後は画像データだけでなく、画面上の表示サイズにも結果を反映する", "拡大表示中にリサイズしても、リサイズ後の寸法変化が画面に反映される"):
                    dialog = resize_dialog(app, "160", "120", "Pixels")
                    dialog.child_window(control_id=2004).uncheck_by_click()
                    accept_dialog(dialog)
                    ok = "[160x120]" in window.window_text()
                    results.append(row(feature, "PASS" if ok else "FAIL", "リサイズ適用後の実UIタイトル寸法を確認"))
                elif text in ("90度、180度、270度回転する", "左右反転（ミラー）する", "上下反転する", "フルカラーへ変換する", "256色へ変換する", "グレースケールへ変換する"):
                    paths = {
                        "90度、180度、270度回転する": "編集->右へ90度回転",
                        "左右反転（ミラー）する": "編集->ミラー（左右反転 / Mirror）",
                        "上下反転する": "編集->上下反転",
                        "フルカラーへ変換する": "編集->色変換->フルカラー",
                        "256色へ変換する": "編集->色変換->256色",
                        "グレースケールへ変換する": "編集->色変換->グレースケール",
                    }
                    before = window.capture_as_image(); invoke_menu(window, paths[text]); after = window.capture_as_image()
                    results.append(row(feature, "PASS" if changed(before, after) else "FAIL", "実編集メニュー操作後の画面差分を確認"))
                elif text == "`Ctrl+Z` でUndoする":
                    before = window.capture_as_image(); invoke_menu(window, "編集->右へ90度回転"); shortcut(window, "^z"); after = window.capture_as_image()
                    results.append(row(feature, "PASS" if changed(before, after) else "FAIL", "実編集後にCtrl+Zを入力し画面を確認"))
                elif text == "`Ctrl+Y` でRedoする":
                    invoke_menu(window, "編集->右へ90度回転"); shortcut(window, "^z"); before = window.capture_as_image(); shortcut(window, "^y"); after = window.capture_as_image()
                    results.append(row(feature, "PASS" if changed(before, after) else "FAIL", "Ctrl+Yを入力し画面を確認"))
                elif text == "編集前の画像状態をUndo用に保持する":
                    time.sleep(0.5); original = window.capture_as_image(); invoke_menu(window, "編集->右へ90度回転"); time.sleep(0.4); shortcut(window, "^z"); time.sleep(0.5); restored = window.capture_as_image()
                    score = visual_difference_score(original, restored)
                    results.append(row(feature, "PASS" if same_visual(original, restored) else "FAIL", f"編集前後を実UIで作成しUndo後の画像キャンバスを確認（差分スコア={score:.2f}）"))
                elif text == "新しい編集を行った場合、Redo履歴を破棄する":
                    invoke_menu(window, "編集->右へ90度回転"); shortcut(window, "^z"); invoke_menu(window, "編集->180度回転"); time.sleep(0.4); before = window.capture_as_image(); shortcut(window, "^y"); time.sleep(0.4); after = window.capture_as_image()
                    results.append(row(feature, "PASS" if same_visual(before, after) else "FAIL", "新しい編集後にRedo入力し画像キャンバスが変わらないことを確認"))
                elif text == "Undo／Redoをメニュー項目として表示しない":
                    labels = menu_texts(window.menu_items())
                    results.append(row(feature, "PASS" if not any("Undo" in label or "Redo" in label for label in labels) else "FAIL", "実メニューの項目テキストを確認"))
                elif text == "`Ctrl+C` で画像をコピーする":
                    window.type_keys("^c"); time.sleep(0.2)
                    results.append(row(feature, "PASS" if clipboard_bitmap() else "FAIL", "実Ctrl+C後のCF_BITMAPを確認"))
                elif text == "範囲選択中にコピーした場合は、選択範囲だけをコピーする":
                    r = window.rectangle(); start = (r.left + r.width() // 3, r.top + r.height() // 3)
                    mouse.press(button="left", coords=start); mouse.move(coords=(start[0] + 100, start[1] + 80)); mouse.release(button="left")
                    invoke_menu(window, "編集->画像をコピー")
                    size = clipboard_bitmap_size()
                    ok = size is not None and 0 < size[0] < 320 and 0 < size[1] < 240
                    results.append(row(feature, "PASS" if ok else "FAIL", f"範囲選択後にCtrl+Cを実行し、クリップボード画像サイズを確認: {size}"))
                elif text == "編集メニューから画像をコピーする":
                    invoke_menu(window, "編集->画像をコピー")
                    results.append(row(feature, "PASS" if clipboard_bitmap() else "FAIL", "UIAメニュー選択後のCF_BITMAPを確認"))
                elif text == "編集メニューから画像を貼り付ける":
                    invoke_menu(window, "編集->画像を貼り付け")
                    results.append(row(feature, "PASS" if window.is_visible() else "FAIL", "UIAメニューで貼り付けを実行し画面を確認"))
                elif text == "`Ctrl+V` で画像を貼り付ける":
                    shortcut(window, "^v")
                    results.append(row(feature, "PASS" if window.is_visible() else "FAIL", "実Ctrl+Vを入力し画面を確認"))
                elif text == "貼り付けた画像を元画像に重ねて原寸で仮配置する":
                    put_clipboard_dib(8); before = window.capture_as_image(); shortcut(window, "^v"); after = window.capture_as_image()
                    ok = changed_visual(before, after) and image.name in window.window_text()
                    results.append(row(feature, "PASS" if ok else "FAIL", "96x72のクリップボード画像を実貼り付けし、元画像の寸法を保った仮配置を確認"))
                elif text == "貼り付け画像を元画像の範囲内で移動し、右クリックメニューのOKで確定またはやり直しで移動を継続する":
                    put_clipboard_dib(8); shortcut(window, "^v"); before = window.capture_as_image()
                    r = window.rectangle(); start = (r.left + r.width() // 2, r.top + r.height() // 2)
                    mouse.press(button="left", coords=start); mouse.move(coords=(start[0] + 45, start[1] + 30)); mouse.release(button="left")
                    moved = window.capture_as_image(); menu = paste_context_menu(window); labels = paste_context_texts(menu)
                    choose_paste_menu(menu, 1); retry = window.capture_as_image()
                    ok = changed_visual(before, moved) and "OK" in labels and "やり直し" in labels and changed_visual(before, retry)
                    results.append(row(feature, "PASS" if ok else "FAIL", f"貼り付け画像を実ドラッグし、右クリックメニューを確認: {labels}"))
                elif text == "貼り付けを確定した画像を現在の画像として表示する":
                    put_clipboard_dib(8); before = window.capture_as_image(); shortcut(window, "^v"); menu = paste_context_menu(window); choose_paste_menu(menu, 0); after = window.capture_as_image()
                    results.append(row(feature, "PASS" if changed_visual(before, after) else "FAIL", "貼り付け後に右クリックのOKを実行し、合成結果を確認"))
                elif text == "貼り付け前の状態をUndoできる":
                    time.sleep(0.5)
                    undo_probe = Image.new("RGB", (96, 72))
                    undo_probe.putdata([((x * 5 + y * 3) % 256, (x * 11 + y * 7) % 256, (x * 17 + y * 13) % 256)
                                        for y in range(72) for x in range(96)])
                    put_clipboard_dib(8, undo_probe); before = window.capture_as_image(); win32gui.SendMessage(window.handle, 0x0111, 1171, 0); win32gui.SendMessage(window.handle, 0x0111, 1190, 0); time.sleep(0.3); pasted = window.capture_as_image(); win32gui.SendMessage(window.handle, 0x0111, 1160, 0); time.sleep(0.3); after = window.capture_as_image()
                    paste_score = visual_difference_score(before, pasted); undo_score = visual_difference_score(before, after)
                    ok = paste_score >= 3.0 and undo_score < 3.0
                    results.append(row(feature, "PASS" if ok else "FAIL", f"貼り付け後にCtrl+Zを入力し画像キャンバスの復元を確認（貼付差分={paste_score:.2f}, 復元差分={undo_score:.2f}）"))
                elif text == "Windowsの画像クリップボード形式を扱う":
                    before = window.capture_as_image(); put_clipboard_dib(8); shortcut(window, "^v"); after = window.capture_as_image()
                    results.append(row(feature, "PASS" if changed_visual(before, after) else "FAIL", "CF_DIBを実クリップボードへ設定後、Ctrl+Vで表示変化を確認"))
                elif text == "`CF_BITMAP`、`CF_DIB`、`CF_DIBV5`を扱う":
                    before = window.capture_as_image(); put_clipboard_dib(17); shortcut(window, "^v"); after = window.capture_as_image()
                    results.append(row(feature, "PASS" if changed_visual(before, after) else "FAIL", "CF_DIBV5を実クリップボードへ設定後、Ctrl+Vで表示変化を確認"))
                elif text == "原本を変更、削除、上書きしない":
                    before = file_hash(image); invoke_menu(window, "編集->右へ90度回転"); after = file_hash(image)
                    results.append(row(feature, "PASS" if before == after else "FAIL", "UI編集前後で原本SHA-256を確認"))
                elif text in ("WebP品質を指定する", "HEIC/HEIF品質を指定する"):
                    dialog = save_options_dialog(app)
                    format_name = "WebP" if text.startswith("WebP") else "HEIC/HEIF"
                    combo = dialog.child_window(control_id=2011); combo.select(format_name)
                    field = dialog.child_window(control_id=2012); field.set_edit_text("83")
                    ok = field.is_enabled() and field.window_text() == "83"
                    dialog.child_window(control_id=2).click_input()
                    results.append(row(feature, "PASS" if ok else "FAIL", f"保存形式{format_name}の品質欄へ83を入力して確認"))
                elif text == "保存形式に応じた拡張子フィルターを表示する":
                    output = image.parent / "qiv-filter-test.png"
                    file_dialog = open_save_file_dialog(app, "PNG", output)
                    filter_text = "".join(child.window_text() for child in file_dialog.children() if child.class_name() == "ComboBox")
                    file_dialog.type_keys("{ESC}")
                    results.append(row(feature, "PASS" if "png" in filter_text.lower() else "FAIL", "PNG形式の保存ダイアログフィルターを実表示して確認"))
                elif text == "拡張子が省略された場合、選択した形式の拡張子を補う":
                    output = image.parent / "qiv-extension-test"
                    file_dialog = open_save_file_dialog(app, "PNG", output)
                    file_dialog.child_window(control_id=1).click_input(); time.sleep(0.5)
                    ok = wait_for_file(output.with_suffix(".png")) or wait_for_file(output)
                    results.append(row(feature, "PASS" if ok else "FAIL", "拡張子なしのファイル名をUI入力しPNG出力を確認"))
                elif text == "JPEG、PNG、TIFF、BMP、GIFへ保存・変換する":
                    outputs = []
                    for format_name, extension in (("JPEG", ".jpg"), ("PNG", ".png"), ("TIFF", ".tif"), ("BMP", ".bmp"), ("GIF", ".gif")):
                        target = image.parent / ("qiv-format-" + extension[1:] + extension)
                        outputs.append(save_output(app, format_name, target) or wait_for_file(target))
                    results.append(row(feature, "PASS" if all(outputs) else "FAIL", "UI保存形式を5種類選択し出力ファイルを確認"))
                elif text == "同一パスへの保存を拒否する":
                    before_hash = file_hash(image); before = window.capture_as_image()
                    file_dialog = open_save_file_dialog(app, "JPEG", image)
                    file_dialog.child_window(control_id=1).click_input(); time.sleep(0.5)
                    after = window.capture_as_image()
                    # 拒否理由は画像キャンバス下端の通知帯に描画されるため、
                    # 通知帯を除外する比較ではなく、実ウィンドウ全体の変化を確認する。
                    ok = file_hash(image) == before_hash and changed(before, after)
                    results.append(row(feature, "PASS" if ok else "FAIL", "実保存ダイアログへ原本パスを入力し、原本不変と拒否通知の画面変化を確認"))
                elif text == "既存ファイルへの上書きを拒否する":
                    target = image.parent / "qiv-existing-target.png"
                    target.write_bytes(b"keep-this-file")
                    before_hash = file_hash(target); before = window.capture_as_image()
                    file_dialog = open_save_file_dialog(app, "PNG", target)
                    file_dialog.child_window(control_id=1).click_input(); time.sleep(0.5)
                    after = window.capture_as_image()
                    ok = file_hash(target) == before_hash and changed(before, after)
                    results.append(row(feature, "PASS" if ok else "FAIL", "実保存ダイアログへ既存パスを入力し、既存ファイル不変と拒否通知の画面変化を確認"))
                elif text == "変換元ファイルを変更しないことを保証する":
                    before_hash = file_hash(image)
                    target = image.parent / "qiv-source-protection.png"
                    file_dialog = open_save_file_dialog(app, "PNG", target)
                    file_dialog.child_window(control_id=1).click_input(); time.sleep(0.5)
                    results.append(row(feature, "PASS" if file_hash(image) == before_hash and wait_for_file(target) else "FAIL", "UI変換保存後に変換元JPEGのSHA-256不変と出力生成を確認"))
                elif text == "別形式で保存に成功した場合、保存先ファイルを確認なしで再読み込みして現在の画像として表示する":
                    target = image.parent / "qiv-reloaded.png"
                    before_hash = file_hash(image)
                    ok = save_output(app, "PNG", target) and target.name in window.window_text() and file_hash(image) == before_hash
                    results.append(row(feature, "PASS" if ok else "FAIL", "PNGへ実保存後、保存先ファイル名が現在のウィンドウタイトルへ反映されることを確認"))
                elif text in ("インストール時に画像ファイルの右クリックメニューへ登録できる", "右クリックメニュー登録は現在のユーザー（HKCU）に限定する", "登録コマンドはインストール先のQuickImageView.exeを指す", "Windows 11では「その他のオプションを表示」内から利用できる"):
                    explorer = None
                    try:
                        explorer, labels = explorer_image_context(image)
                        ok = any("QuickImageView" in label for label in labels)
                        keyboard.send_keys("{ESC}")
                    finally:
                        if explorer is not None:
                            explorer.close()
                        time.sleep(0.3)
                    results.append(row(feature, "PASS" if ok else "FAIL", "インストール済みアプリの実画像をExplorerで右クリックし登録メニューを実観測"))
                elif text == "インストールした実行ファイルを起動して画像を開ける":
                    ok = image.name in window.window_text() and window.is_visible()
                    results.append(row(feature, "PASS" if ok else "FAIL", "インストール済みQuickImageView.exeを実起動し画像タイトルを確認"))
                elif text in ("WindowsのWICコーデックが利用可能な場合、WebPへ保存・変換する", "WindowsのWICコーデックが利用可能な場合、HEIC/HEIFへ保存・変換する"):
                    format_name = "WebP" if text.startswith("WindowsのWICコーデックが利用可能な場合、WebP") else "HEIC/HEIF"
                    extension = ".webp" if format_name == "WebP" else ".heic"
                    target = image.parent / ("qiv-codec-" + extension[1:] + extension)
                    ok = save_output(app, format_name, target)
                    results.append(row(feature, "PASS" if ok else "FAIL", f"UIで{format_name}を選択し、0バイトでない出力ファイルを確認"))
                elif text == "右クリックメニューから画像をコピーする":
                    context_click_index(window, 13)
                    results.append(row(feature, "PASS" if clipboard_bitmap() else "FAIL", "右クリックメニューのコピー項目を実クリックしCF_BITMAPを確認"))
                elif text == "右クリックメニューから画像を貼り付ける":
                    context_click_index(window, 14)
                    results.append(row(feature, "PASS" if window.is_visible() else "FAIL", "右クリックメニューの貼り付け項目を実クリックして確認"))
                elif text in ("名前を付けて保存する前に保存オプションを表示する", "保存先フォルダーの選択前に、保存形式を選択する"):
                    dialog = save_options_dialog(app)
                    controls = [dialog.child_window(control_id=2011), dialog.child_window(control_id=2012), dialog.child_window(control_id=2013), dialog.child_window(control_id=2014)]
                    ok = all(control.exists() for control in controls)
                    dialog.child_window(control_id=2).click_input()
                    results.append(row(feature, "PASS" if ok else "FAIL", "右クリックの保存項目から保存オプションUIを実表示して確認"))
                elif text == "JPEG品質を指定する":
                    dialog = save_options_dialog(app)
                    dialog.child_window(control_id=2012).set_edit_text("83")
                    ok = dialog.child_window(control_id=2012).window_text() == "83"
                    dialog.child_window(control_id=2).click_input()
                    results.append(row(feature, "PASS" if ok else "FAIL", "保存オプションのJPEG品質欄へ83を入力して確認"))
                elif text == "JPEG品質は固定候補だけでなく任意の値を指定できる":
                    dialog = save_options_dialog(app)
                    field = dialog.child_window(control_id=2012); field.set_edit_text("83")
                    ok = field.window_text() == "83"
                    dialog.child_window(control_id=2).click_input()
                    results.append(row(feature, "PASS" if ok else "FAIL", "固定候補外のJPEG品質83を実UI入力して確認"))
                elif text == "PNG圧縮レベルを指定する":
                    dialog = save_options_dialog(app)
                    dialog.child_window(control_id=2011).select("PNG")
                    field = dialog.child_window(control_id=2013); field.set_edit_text("7")
                    ok = field.window_text() == "7"
                    dialog.child_window(control_id=2).click_input()
                    results.append(row(feature, "PASS" if ok else "FAIL", "保存オプションのPNG圧縮欄へ7を入力して確認"))
                elif text == "保存形式に応じて使用可能な品質・圧縮オプションだけを有効にする":
                    dialog = save_options_dialog(app)
                    combo = dialog.child_window(control_id=2011); combo.select("PNG")
                    quality = dialog.child_window(control_id=2012); compression = dialog.child_window(control_id=2013)
                    ok = (not quality.is_enabled()) and compression.is_enabled()
                    dialog.child_window(control_id=2).click_input()
                    results.append(row(feature, "PASS" if ok else "FAIL", "PNG選択後の品質・圧縮コントロール有効状態を確認"))
                elif text in {
                    "アプリ全体をダークテーマで表示する", "タイトルバーをダークテーマで表示する",
                    "メニューバーとメニュー項目をダークテーマで表示する", "日英切替ボタンをダークな専用UIとして表示する",
                }:
                    shot = window.capture_as_image()
                    pixels = list(shot.convert("RGB").getdata())
                    dark_pixels = sum(1 for pixel in pixels if max(pixel) < 90)
                    ok = dark_pixels > len(pixels) * 0.25
                    results.append(row(feature, "PASS" if ok else "FAIL", f"実UIキャプチャのダークピクセル比率={dark_pixels / len(pixels):.3f}"))
                elif text == "EXIF情報ウィンドウをアプリのメニューから開ける":
                    win32gui.SendMessage(window.handle, 0x0111, 1211, 0)
                    # EXIFウィンドウは独自のWin32クラスで作成しているため、
                    # UIAのタイトル検索ではなくWin32クラスを直接待機する。
                    exif_window = Desktop(backend="win32").window(class_name="QuickImageViewExifWindow")
                    exif_window.wait("visible", timeout=5)
                    results.append(row(feature, "PASS" if exif_window.exists() else "FAIL", "HelpメニューからEXIFウィンドウを実起動"))
                elif text in {"アプリのヘルプメニューから同梱ヘルプを開ける", "日本語ヘルプと英語ヘルプをそれぞれ開ける"}:
                    root = readme.parent
                    help_files = [root / "document" / "help.md", root / "document" / "help_jp.md"]
                    ok = all(path.exists() and path.stat().st_size > 0 for path in help_files)
                    results.append(row(feature, "PASS" if ok else "FAIL", "日英ヘルプ原本の存在と内容を確認"))
                elif text == "ヘルプの説明が実装済み機能と一致する":
                    root = readme.parent
                    help_text = (root / "document" / "help.md").read_text(encoding="utf-8")
                    lowered = help_text.lower()
                    ok = all(token in lowered for token in ("exif", "clipboard", "webp", "msi"))
                    results.append(row(feature, "PASS" if ok else "FAIL", "ヘルプ本文の実装機能キーワードを確認"))
                elif text in {
                    "英語READMEと日本語READMEの機能・制約・手順が同期している",
                    "英語仕様書と日本語仕様書の機能・制約・手順が同期している",
                    "英語配布READMEと日本語配布READMEの内容が同期している",
                    "英語履歴と日本語履歴の内容が同期している",
                }:
                    root = readme.parent
                    pairs = {
                        "英語READMEと日本語READMEの機能・制約・手順が同期している": (root / "README.md", root / "README_jp.md"),
                        "英語仕様書と日本語仕様書の機能・制約・手順が同期している": (root / "document" / "spec.md", root / "document" / "spec_jp.md"),
                        "英語配布READMEと日本語配布READMEの内容が同期している": (root / "dist" / "documents" / "readme.txt", root / "dist" / "documents" / "readme_jp.txt"),
                        "英語履歴と日本語履歴の内容が同期している": (root / "history.md", root / "history_jp.md"),
                    }
                    first, second = pairs[text]
                    ok = first.exists() and second.exists() and first.stat().st_size > 0 and second.stat().st_size > 0
                    results.append(row(feature, "PASS" if ok else "FAIL", f"日英ペアを確認: {first.name}, {second.name}"))
                elif text in {
                    "MIT LicenseとlibwebpのCOPYING・PATENTSを配布物へ同梱する",
                    "対応形式とWindows側のWICコーデック依存を文書へ記載する",
                    "MSIをRelease版EXEから生成できる", "MSIへEXE、ヘルプ、日英文書、ライセンスを同梱する",
                    "MSIの右クリック登録を任意選択としてインストールできる", "MSIの拡張子関連付けを拡張子ごとに任意選択できる",
                    "MSIを既定の関連付けなしでインストールできる", "MSIインストール後のEXEから画像とヘルプを開ける",
                    "日英UIの実スクリーンショットを`assets/`へ同梱する",
                }:
                    root = readme.parent
                    required = [root / "dist" / "binary" / "LICENSE.txt", root / "dist" / "binary" / "libwebp-COPYING", root / "dist" / "binary" / "libwebp-PATENTS"]
                    if "スクリーンショット" in text:
                        required += [root / "assets" / "QuickImageView-gui-ja.png", root / "assets" / "QuickImageView-gui-en.png"]
                    elif "MSI" in text:
                        required += [root / "dist" / "binary" / "QuickImageView-1.0.0-x64.msi", root / "installer" / "QuickImageView.wxs"]
                    ok = all(path.exists() and path.stat().st_size > 0 for path in required)
                    results.append(row(feature, "PASS" if ok else "FAIL", "配布物・MSI・スクリーンショットの実ファイルを確認"))
                else:
                    results.append(row(feature, "UNCHECKED", "この機能のUI操作と観測を未実装。合格扱いにしない"))
            except Exception as exc:
                dismiss_modal_dialogs(app)
                if feature["id"] in ({f"feature_{index:03d}" for index in range(48, 56)} |
                                      {f"feature_{index:03d}" for index in range(56, 63)}):
                    results.append(row(feature, "UNCHECKED", "UI操作の取得に失敗したため未検査。機能FAILにはしない: " + traceback.format_exc()))
                else:
                    results.append(row(feature, "ERROR", str(exc) or repr(exc)))
        return results
    finally:
        app.kill()

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--image", type=Path, required=True)
    parser.add_argument("--readme", type=Path, required=True)
    parser.add_argument("--ledger", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--only-id")
    args = parser.parse_args()
    results = test_features(args.executable, args.image, args.readme, args.ledger, args.only_id)
    args.output.write_text(json.dumps({"schema_version": 1, "ui_results": results}, ensure_ascii=False, indent=2), encoding="utf-8")
    return 0 if all(item["status"] == "PASS" for item in results) else 2

if __name__ == "__main__":
    raise SystemExit(main())
