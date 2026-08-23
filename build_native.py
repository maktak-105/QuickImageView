"""Quick app build wrapper for the native CMake project."""
from pathlib import Path
import subprocess
import sys
import shutil

ROOT = Path(__file__).resolve().parent
BUILD = ROOT / "dist" / "binary"

def run(*args: str) -> None:
    result = subprocess.run(args, cwd=ROOT)
    if result.returncode:
        raise SystemExit(result.returncode)

run("cmake", "-S", ".", "-B", str(BUILD), "-G", "MinGW Makefiles", "-DCMAKE_BUILD_TYPE=Release")
run("cmake", "--build", str(BUILD), "--clean-first")
shutil.copy2(ROOT / "document" / "help.md", BUILD / "help.md")
shutil.copy2(ROOT / "document" / "help_jp.md", BUILD / "help_jp.md")
