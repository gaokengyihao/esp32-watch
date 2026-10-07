# ============================================================
#  手表项目一键同步（拉取 -> 提交 -> 推送）
#  用法：双击同目录下的 sync.bat
# ============================================================
$ErrorActionPreference = 'Stop'
try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch {}
Set-Location $PSScriptRoot

function Pause-Exit([int]$code) {
    Write-Host ''
    Read-Host '按回车键关闭'
    exit $code
}

Write-Host '============================================'
Write-Host '  手表项目一键同步（拉取 + 提交 + 推送）'
Write-Host '============================================'
Write-Host ''

try {
    # [1/4] 拉取远程改动
    Write-Host '[1/4] 拉取远程改动 ...'
    git pull --rebase --autostash
    if ($LASTEXITCODE -ne 0) { throw 'git pull 失败：可能是网络不通或存在冲突，请把上面的信息发给 AI 处理' }

    # [2/4] 查看本地改动
    Write-Host ''
    Write-Host '[2/4] 本地改动：'
    $changes = git status --short
    if ($changes) { $changes } else { Write-Host '  （无）' }

    # 安全闸门：禁止把非空 WiFi 密码提交进仓库
    git add -A
    $staged = git diff --cached | Out-String
    if ($staged -match 'CONFIG_WATCH_WIFI_PASSWORD\s*=\s*"[^"]+') {
        Write-Host ''
        Write-Host '[警告] 暂存区出现非空的 WiFi 密码，已停止提交！' -ForegroundColor Red
        Write-Host '       密码只能写在 sdkconfig（已被忽略）；sdkconfig.defaults 必须留空。'
        Pause-Exit 1
    }

    # [3/4] 提交并推送
    git diff --cached --quiet
    if ($LASTEXITCODE -ne 0) {
        $msg = 'sync: ' + (Get-Date -Format 'yyyy-MM-dd HH:mm')
        git commit -m $msg | Out-Null
        Write-Host ''
        Write-Host "[3/4] 已提交：$msg，正在推送 ..."
        git push
        if ($LASTEXITCODE -ne 0) { throw 'git push 失败：检查网络（GitHub 可能需要代理）' }
    } else {
        Write-Host ''
        Write-Host '[3/4] 没有新改动，无需提交'
    }

    Write-Host ''
    Write-Host '[4/4] 同步完成 √' -ForegroundColor Green
    Pause-Exit 0
}
catch {
    Write-Host ''
    Write-Host "[失败] $_" -ForegroundColor Red
    Pause-Exit 1
}
