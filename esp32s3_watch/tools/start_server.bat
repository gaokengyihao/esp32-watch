@echo off
chcp 65001 >nul
echo ==========================================
echo   ESP32 Watch - firmware HTTP server
echo   (OTA / mini program update page uses it)
echo ==========================================
echo.
echo   URL:  http://192.168.1.174:8000/
echo   Dir:  D:\esp32\ota_server
echo.
echo   *** keep this window open ***
echo.
cd /d D:\esp32\ota_server
"C:\Espressif\tools\python\v6.1-beta1\venv\Scripts\python.exe" -m http.server 8000
pause
