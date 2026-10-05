@echo off
chcp 65001 >nul
echo ==========================================
echo   ESP32 Watch - firmware HTTP server
echo   (OTA / mini program update page uses it)
echo ==========================================
echo.
echo   URL:  http://<your PC IP>:8000/     (ipconfig to check)
echo   Dir:  %~dp0..\..\ota_server
echo.
echo   *** keep this window open ***
echo.
cd /d "%~dp0..\..\ota_server"
python -m http.server 8000
pause
