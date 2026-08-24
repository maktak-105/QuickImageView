"""Build and test the Qt Quick migration target with the matching Qt toolchain."""

from __future__ import annotations

import os
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build" / "qt-mingw1310"
QT_ROOT = Path(os.environ.get("QT_ROOT", r"C:\Qt\6.10.3\mingw_64"))
MINGW_BIN = Path(os.environ.get("QT_MINGW_BIN", r"C:\Qt\Tools\mingw1310_64\bin"))
QT_CMAKE = QT_ROOT / "bin" / "qt-cmake.bat"
QT_DEPLOY = QT_ROOT / "bin" / "windeployqt.exe"
QT_BINARY = ROOT / "dist" / "binary" / "QuickImageViewQt.exe"


def require_file(path: Path, description: str) -> None:
    if not path.is_file():
        raise SystemExit(f"{description} was not found: {path}")


def run(command: list[str], env: dict[str, str]) -> None:
    result = subprocess.run(command, cwd=ROOT, env=env)
    if result.returncode:
        raise SystemExit(result.returncode)


require_file(QT_CMAKE, "Qt CMake wrapper")
require_file(MINGW_BIN / "g++.exe", "Qt MinGW compiler")
require_file(QT_DEPLOY, "Qt deployment tool")

environment = os.environ.copy()
environment["PATH"] = str(MINGW_BIN) + os.pathsep + str(QT_ROOT / "bin") + os.pathsep + environment["PATH"]

run([
    "cmd.exe", "/c", "call", str(QT_CMAKE),
    "-S", ".", "-B", str(BUILD), "-G", "MinGW Makefiles",
    "-DCMAKE_BUILD_TYPE=Release", "-DQUICKIMAGEVIEW_BUILD_QT=ON",
], environment)
run([
    "cmake", "--build", str(BUILD), "--target",
    "QuickImageViewQt", "qiv_controller_tests", "qiv_qml_tests", "--parallel", "2",
], environment)
run(["ctest", "--test-dir", str(BUILD), "--output-on-failure"], environment)

require_file(QT_BINARY, "Qt build output")

run([
    str(QT_DEPLOY), "--release", "--qmldir", str(ROOT / "qml"), "--no-translations", str(QT_BINARY),
], environment)
