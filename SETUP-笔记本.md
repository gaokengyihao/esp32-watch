# 宿舍笔记本首次搭建（一次性，约 30 分钟）

> 前提：公司电脑已把仓库推送到 GitHub。
> 下面命令都在 **PowerShell** 里执行（Win 开始菜单搜索 "PowerShell"）。

## 0. 先装好这些软件

| 软件 | 建议安装位置 | 说明 |
|---|---|---|
| Git for Windows | 默认 | https://git-scm.com/download/win |
| ESP-IDF v6.1-beta1 | `C:\esp\v6.1-beta1\esp-idf` | 与公司电脑路径一致最省事 |
| Flutter SDK | `D:\flutter` | 与公司电脑路径一致 |
| VS Code + ESP-IDF 插件 | 默认 | 可选，但推荐 |
| 微信开发者工具 | 默认 | 只有要改小程序时才需要 |

> 路径不一致也能用，之后改 `watch\esp32s3_watch\.vscode\settings.json` 里的 IDF 路径即可。

## 1. 配置 Git 身份

```powershell
git config --global user.name "gaokengyihao"
git config --global user.email "312788247+gaokengyihao@users.noreply.github.com"
git config --global init.defaultBranch main
git config --global core.quotepath false
```

## 2. 登录 GitHub（二选一）

**方式 A（推荐）**：装 gh 后浏览器登录，之后不再问密码

```powershell
winget install --id GitHub.cli -e
gh auth login        # 选 GitHub.com -> HTTPS -> 浏览器登录
```

**方式 B**：不装 gh，首次 push 时也会弹出浏览器登录窗口。

> 如果 GitHub 访问慢，可挂代理（改成自己代理端口）：
> `git config --global http.proxy http://127.0.0.1:7897`

## 3. 克隆仓库到 D:\esp32

```powershell
git clone https://github.com/gaokengyihao/esp32-watch.git D:\esp32
```

> `D:\esp32` 必须是空的或不存在。克隆后目录结构与公司电脑一致（`watch\` 里是四个工程）。

## 4. 固件首次编译

```powershell
cd D:\esp32\watch\esp32s3_watch
idf.py menuconfig      # 进 "Watch 手表配置"，填宿舍 WiFi 的 SSID 和密码
idf.py build
```

> `managed_components` 会自动重新下载（约 180MB，需要能访问组件服务器，耐心等一次）。

## 5. Flutter App 首次运行

```powershell
cd D:\esp32\watch\watch_app
flutter pub get
flutter run -d windows
```

## 6. 小程序（要改时）

微信开发者工具 → 导入项目 → 选 `D:\esp32\watch\watch_mp`

## 7. 以后每天

双击 `D:\esp32\sync.bat` —— 完成。

## 常见问题

- **push 很慢/失败**：GitHub 网络问题，挂代理或用手机热点。
- **编译报找不到 managed_components**：正常，首次 build 自动下载；确认网络能访问 components.espressif.com。
- **VSCode 报找不到 ESP-IDF**：改 `watch\esp32s3_watch\.vscode\settings.json` 里的路径为你本机安装位置。
- **WiFi 对时失败**：`idf.py menuconfig` 里检查 SSID/密码（必须是 2.4G 频段 WiFi）。
- **资料在哪里**：`资料\` 文件夹（芯片手册、屏资料）不在仓库里，用 U 盘/网盘从公司电脑拷一次即可。
