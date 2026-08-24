"""Build QuickImageView with disposable intermediates and a single binary output."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build" / "native"
DIST = ROOT / "dist" / "binary"

def run(*args: str) -> None:
    result = subprocess.run(args, cwd=ROOT)
    if result.returncode:
        raise SystemExit(result.returncode)

run("cmake", "-S", ".", "-B", str(BUILD), "-G", "MinGW Makefiles", "-DCMAKE_BUILD_TYPE=Release")
run("cmake", "--build", str(BUILD), "--clean-first")
if not (DIST / "QuickImageView.exe").is_file():
    raise SystemExit(f"Build output not found: {DIST / 'QuickImageView.exe'}")
