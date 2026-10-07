@echo off
chcp 65001 >nul
echo ==========================================
echo   ESP32 Watch - build and publish firmware
echo ==========================================
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0publish.ps1"
echo.
pause
