@echo off
if /I "%~1"=="qt" (
  shift
  python "%~dp0build-tools\build_qt.py" %*
  exit /b
)
python "%~dp0build-tools\build_native.py" %*
