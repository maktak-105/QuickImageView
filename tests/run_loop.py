from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import tempfile
import sys
import psutil
from pathlib import Path

from PIL import Image
from PIL.ExifTags import TAGS
from catalog import load_and_validate

INSTALL_TIME_FEATURES = {
    "インストール時に画像ファイルの右クリックメニューへ登録できる",
    "右クリックメニュー登録は現在のユーザー（HKCU）に限定する",
    "登録コマンドはインストール先のQuickImageView.exeを指す",
    "Windows 11では「その他のオプションを表示」内から利用できる",
    "登録なしでインストールできるオプションを用意する",
    "アンインストール時はQuickImageView専用の登録だけを削除する",
    "他のアプリケーションの登録を削除しない",
}

def make_exif_jpeg(path: Path) -> None:
    image = Image.new("RGB", (320, 240))
    pixels = image.load()
    for y in range(image.height):
        for x in range(image.width):
            pixels[x, y] = ((x + 30) % 256, (y + 60) % 256, (x + y + 90) % 256)
    exif = image.getexif()
    exif[271] = "QuickImageView Test Maker"
    exif[272] = "QIV-EXIF-01"
    exif[306] = "2026:08:23 10:00:00"
    image.save(path, format="JPEG", quality=92, exif=exif.tobytes())

def command(name: str, args: list[str]) -> dict:
    completed = subprocess.run(args, text=True, capture_output=True)
    return {"name": name, "status": "PASS" if completed.returncode == 0 else "FAIL", "exit_code": completed.returncode, "output": (completed.stdout + completed.stderr).strip()}

def hash_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest().upper()

def kill_exact_executable(executable: Path) -> None:
    target = str(executable.resolve()).lower()
    for process in psutil.process_iter(["pid", "exe"]):
        try:
            if process.info.get("exe") and str(Path(process.info["exe"]).resolve()).lower() == target:
                process.kill()
        except (psutil.NoSuchProcess, psutil.AccessDenied, OSError):
            pass

def run_ui_isolated(executable: Path, image: Path, readme: Path, ledger: Path, features: list[dict], temp: Path) -> list[dict]:
    results = []
    script = Path(__file__).with_name("ui_test.py")
    for feature in features:
        result_path = temp / (feature["id"] + ".json")
        command_line = [sys.executable, str(script), "--executable", str(executable), "--image", str(image), "--readme", str(readme), "--ledger", str(ledger), "--output", str(result_path), "--only-id", feature["id"]]
        try:
            timeout = 45 if feature["text"] in {
                "JPEG、PNG、TIFF、BMP、GIFへ保存・変換する",
            } else 20
            subprocess.run(command_line, timeout=timeout, capture_output=True, text=True)
            if result_path.exists():
                data = json.loads(result_path.read_text(encoding="utf-8"))
                results.extend(data.get("ui_results", []))
            else:
                results.append({**feature, "status": "ERROR", "detail": "UI検査子プロセスが結果を出力しませんでした", "ui": True})
        except subprocess.TimeoutExpired:
            kill_exact_executable(executable)
            results.append({**feature, "status": "ERROR", "detail": f"このUI操作が{timeout}秒を超過したため子プロセスを終了しました", "ui": True})
    return results

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    root = args.root.resolve()
    build = root / "build"
    exe = build / "QuickImageView.exe"
    readme = root / "README.md"
    ledger = root / "tests" / "operations.json"
    all_features = load_and_validate(readme, ledger)
    EXCLUDED_FROM_APP_UI = {feature["id"] for feature in all_features
                            if feature["text"] in INSTALL_TIME_FEATURES}
    catalog = [feature for feature in all_features if feature["id"] not in EXCLUDED_FROM_APP_UI]
    temp = Path(tempfile.mkdtemp(prefix="QuickImageView-python-loop-"))
    install = temp / "installed"
    image = temp / "input.jpg"
    make_exif_jpeg(image)
    steps = []
    supplemental = []
    try:
        steps.append(command("configure", ["cmake", "-S", str(root), "-B", str(build), "-G", "MinGW Makefiles"]))
        steps.append(command("build", ["cmake", "--build", str(build), "--clean-first"]))
        if steps[-1]["status"] != "PASS":
            raise RuntimeError("ビルド失敗のためインストールとUI検査を開始しません")

        install_cmd = ["pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(root / "scripts" / "install.ps1"), "-SourceExe", str(exe), "-InstallDirectory", str(install)]
        steps.append(command("install", install_cmd))
        installed = install / "QuickImageView.exe"
        parity = installed.exists() and hash_file(exe) == hash_file(installed)
        steps.append({"name": "install_parity", "status": "PASS" if parity else "FAIL", "exit_code": 0 if parity else 1, "output": f"build={hash_file(exe) if exe.exists() else ''} install={hash_file(installed) if installed.exists() else ''}"})

        # UI検査はインストール済みアプリに対して一度だけ実行する。
        ui_results = run_ui_isolated(installed, image, readme, ledger, catalog, temp)
        steps.append({"name": "ui_installed", "status": "PASS" if all(row["status"] == "PASS" for row in ui_results) else "INCOMPLETE", "exit_code": 0 if all(row["status"] == "PASS" for row in ui_results) else 2, "output": "インストール済みアプリで全台帳行を実行"})

        # UI結果を確定した後だけ補助検査を行う。
        supplemental.append(command("ctest", ["ctest", "--test-dir", str(build), "--output-on-failure"]))
        supplemental.append(command("self_test", [str(exe), "--self-test"]))
    except Exception as exc:
        steps.append({"name": "runner", "status": "ERROR", "exit_code": 1, "output": str(exc)})
        ui_results = [{**row, "status": "ERROR", "detail": "UI検査を開始できません: " + str(exc)} for row in catalog]
    finally:
        uninstall = root / "scripts" / "uninstall.ps1"
        if uninstall.exists():
            subprocess.run(["pwsh", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(uninstall), "-InstallDirectory", str(install)], capture_output=True, text=True)

    payload = {"schema_version": 1, "source": "README.md -> tests/operations.json", "ui_first": True, "excluded_from_app_ui": sorted(EXCLUDED_FROM_APP_UI), "flow": ["build", "install", "ui_installed", "supplemental"], "ui_results": ui_results, "steps": steps, "supplemental": supplemental}
    output = root / "docs" / "loop" / "current.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    report = root / "docs" / "loop" / "report.html"
    rows = "".join(f"<tr class='{r['status'].lower()}'><td>{r['id']}</td><td>{r['section']}</td><td>{r['text']}</td><td>{r['status']}</td><td>{r['detail']}</td></tr>" for r in ui_results)
    report.write_text("<!doctype html><meta charset='utf-8'><title>QuickImageView UI検査</title><style>body{font-family:Segoe UI}table{border-collapse:collapse}td,th{border:1px solid #999;padding:5px}.pass{background:#cfc}.fail,.error{background:#fcc}.unchecked{background:#ffc}</style><h1>QuickImageView UI検査</h1><table><tr><th>ID</th><th>区分</th><th>README機能</th><th>結果</th><th>詳細</th></tr>" + rows + "</table>", encoding="utf-8")
    return 0 if all(r["status"] == "PASS" for r in ui_results) and all(s["status"] == "PASS" for s in steps) else 2

if __name__ == "__main__":
    raise SystemExit(main())
