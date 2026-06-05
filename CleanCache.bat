@echo off
setlocal

REM Double-click or run from repo root to clear Unreal build caches.
REM Close Unreal Editor before running.

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\Clean-UnrealCache.ps1" %*
set "EXIT_CODE=%ERRORLEVEL%"

if not "%EXIT_CODE%"=="0" (
    echo.
    echo Clean failed with exit code %EXIT_CODE%.
    pause
    exit /b %EXIT_CODE%
)

echo.
pause
exit /b 0
