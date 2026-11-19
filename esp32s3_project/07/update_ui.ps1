# update_ui.ps1
# 用法：在 ESP-IDF 终端里，进入 07 目录后运行
#   .\update_ui.ps1

$ErrorActionPreference = "Stop"

$Project = $PSScriptRoot
$Src     = Join-Path $Project "components\ui_files\src"
$Assets  = "D:\esp32\lvgl\lvgl_project\03_lvgl_ui\assets\fonts"

Set-Location $Project

Write-Host ""
Write-Host "=== 1/3 清空旧的 UI 生成文件 ===" -ForegroundColor Cyan
if (Test-Path $Src) {
    Get-ChildItem $Src -Force | Remove-Item -Recurse -Force
    Write-Host "已清空: $Src"
} else {
    New-Item -ItemType Directory -Force $Src | Out-Null
}

Write-Host ""
Write-Host "=== 2/3 请在 SquareLine 里导出 ===" -ForegroundColor Yellow
Write-Host "导出路径已经设置为："
Write-Host "  $Src" -ForegroundColor Green
Write-Host ""
Write-Host "在 SquareLine 点击：File -> Export UI files"
Write-Host "导出完成后回到这里，按回车继续..."
Read-Host

Write-Host ""
Write-Host "=== 检查字体 ===" -ForegroundColor Cyan
$FontDst = Join-Path $Src "fonts"
if (-not (Test-Path $FontDst)) {
    New-Item -ItemType Directory -Force $FontDst | Out-Null
}
Get-ChildItem $Assets -Filter "ui_font_*.c" -ErrorAction SilentlyContinue | ForEach-Object {
    $target = Join-Path $FontDst $_.Name
    if (-not (Test-Path $target)) {
        Copy-Item $_.FullName $target
        Write-Host "补上字体: $($_.Name)"
    }
}

Write-Host ""
Write-Host "=== 3/3 重新配置并编译 ===" -ForegroundColor Cyan
idf.py reconfigure
idf.py build

Write-Host ""
Write-Host "完成。可以烧录：" -ForegroundColor Green
Write-Host "  idf.py -p COM8 flash monitor"
