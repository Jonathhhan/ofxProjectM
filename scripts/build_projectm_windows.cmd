@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
powershell -ExecutionPolicy Bypass -File "%SCRIPT_DIR%build_projectm_windows.ps1" %*

if errorlevel 1 (
	exit /b %errorlevel%
)

exit /b 0
