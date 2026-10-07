# ESP32-S3 智能手表（个人开发仓库）

固件 + Flutter App + 微信小程序 + OTA 文件，**公司电脑 ↔ 宿舍笔记本**通过本仓库同步。

## 目录地图（一眼看懂哪些会同步）

```
D:\esp32\
├─ esp32s3_watch\        ✅ 同步   ESP32-S3 固件（ESP-IDF 工程）
├─ watch_app\            ✅ 同步   Flutter App（电脑调试工具）
├─ watch_mp\             ✅ 同步   微信小程序（手机端）
├─ ota_server\           ✅ 同步   OTA 服务器文件（version.txt）
├─ fw_release\           🚫 本机   固件发布包（.bin 用 U 盘/网盘拷）
├─ 资料\                 🚫 本机   芯片资料 / 屏幕资料 / 参考工程 / 驱动
├─ 其他\                 🚫 本机   练习工程、杂项
├─ sync.bat / sync.ps1          一键同步脚本
└─ README.md / SETUP-笔记本.md
```

**规则**：项目文件夹里"人写的"文件全部同步；"机器生成的"（`build/`、`managed_components/`、`.dart_tool/`）和"密码文件"（`sdkconfig`）不同步——它们会自动重建，所以仓库里 = 完整可编译的项目，一个文件不少。

## 日常同步（唯一需要记住的事）

双击根目录的 **`sync.bat`**：自动「拉取 → 提交 → 推送」，只传变化的文件（通常几 KB）。

## 不会同步的细节

- `build/`、`managed_components/`、`.dart_tool/` —— 编译产物，编译时自动重建
- `sdkconfig` —— **含 WiFi 密码，各机私有**；新电脑第一次用 `idf.py menuconfig -> Watch 手表配置` 填自己的 WiFi
- `*.bin` —— 固件发布包，含密码明文且每次编译都变，用 U 盘/网盘拷
- `资料\`、`其他\` —— 本机专用文件夹（里面有 `说明.txt`），不参与同步

## 新电脑首次搭建

见 [SETUP-笔记本.md](SETUP-笔记本.md)。

## 给协作者开权限

GitHub 仓库页面 → **Settings → Collaborators → Add people**，仅被邀请的人可见。

## 注意

- 不要往被跟踪的文件里写密码/密钥；WiFi 密码只写在 `sdkconfig`（已忽略）里。
- `sync.bat` 内置密码闸门：暂存区出现非空的 `CONFIG_WATCH_WIFI_PASSWORD` 会拒绝提交。
