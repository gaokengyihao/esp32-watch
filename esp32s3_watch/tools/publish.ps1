# publish.ps1 —— 编译 + 发布固件（旧的自动删除）
#
# 用法：双击 tools\publish.bat（推荐），或在 PowerShell 里运行本脚本
# 做完三件事：
#   1) idf.py build
#   2) 把 build\esp32s3_watch.bin 复制到 D:\esp32\fw_release\esp32s3_watch_v<版本>.bin
#      （发布目录里的旧 .bin 自动删掉）
#   3) 顺便同步 WiFi-OTA 的服务器目录（D:\esp32\ota_server，version.txt 一起更新）

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot        # 项目根目录（tools 的上一级）
Set-Location $root
[Console]::OutputEncoding = [Text.Encoding]::UTF8

# 0) 环境里没有 idf.py 的话，自动加载一次 ESP-IDF 环境
if (-not (Get-Command idf.py -ErrorAction SilentlyContinue)) {
    $profile = 'C:\Espressif\tools\Microsoft.v6.1-beta1.PowerShell_profile.ps1'
    if (Test-Path $profile) {
        . $profile | Out-Null
        Write-Host '已加载 ESP-IDF 环境' -ForegroundColor DarkGray
    } else {
        throw '找不到 idf.py，也不是默认 IDF 环境，请先打开 IDF 终端'
    }
}

# 1) 从源码读版本号（main/watch_version.h 里的 #define 那一行）
$h = Get-Content 'main\watch_version.h' -Raw
if ($h -match '#define\s+WATCH_FW_VERSION\s+"([0-9\.]+)"') { $ver = $Matches[1] } else { throw '读不到版本号' }
Write-Host "固件版本: v$ver" -ForegroundColor Cyan

# 2) 编译
Write-Host '编译中...' -ForegroundColor Cyan
$buildOut = idf.py build 2>&1
$buildOut | Select-String -Pattern 'error|FAILED|binary size|Project build complete' | ForEach-Object { $_.Line }
if ($LASTEXITCODE -ne 0) { throw '编译失败，先修错误再发布' }

# 3) 发布到指定目录（旧 .bin 自动删除）
$rel = 'D:\esp32\fw_release'
New-Item -ItemType Directory -Force $rel | Out-Null
Remove-Item "$rel\*.bin" -Force -ErrorAction SilentlyContinue
$dst = "$rel\esp32s3_watch_v$ver.bin"
Copy-Item 'build\esp32s3_watch.bin' $dst -Force
Write-Host "已发布: $dst" -ForegroundColor Green

# 4) 同步 WiFi-OTA 服务器目录（手表自己联网升级用的那套）
$srv = 'D:\esp32\ota_server'
if (Test-Path $srv) {
    Copy-Item 'build\esp32s3_watch.bin' "$srv\esp32s3_watch.bin" -Force
    Set-Content "$srv\version.txt" -Value $ver -NoNewline -Encoding ascii
    Write-Host "已同步 WiFi-OTA 服务器: version.txt = $ver" -ForegroundColor Green
}

Write-Host ''
Write-Host '下一步：' -ForegroundColor Yellow
Write-Host "  · 电脑 HTTP 服务器起着的话（tools\start_server.bat），小程序一打开就会自动检测到 v$ver" -ForegroundColor Yellow
Write-Host '  · 没在跑就先双击 tools\start_server.bat（窗口别关）' -ForegroundColor Yellow
Write-Host '  · 小程序升级页：没有新版本就没有按钮；有新版本才会出现"立即升级"按钮 ✓' -ForegroundColor Yellow
