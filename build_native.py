"""Quick app build wrapper for the native CMake project."""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
BUILD = ROOT / "build"

def run(*args: str) -> None:
    result = subprocess.run(args, cwd=ROOT)
    if result.returncode:
        raise SystemExit(result.returncode)

run("cmake", "-S", ".", "-B", str(BUILD), "-G", "MinGW Makefiles")
run("cmake", "--build", str(BUILD), "--clean-first")
