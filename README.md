# ESP32-S3 智能手表（个人开发仓库）

**公司电脑 ↔ 宿舍笔记本**通过本仓库同步。固件 + Flutter App + 微信小程序 + OTA，全部收在 `watch\` 里。

## 目录地图（一眼看懂哪些会同步）

```
D:\esp32\
├─ watch\                     ✅ 同步      ← 手表项目（整个文件夹都是）
│   ├─ esp32s3_watch\                固件（ESP-IDF 工程）
│   ├─ watch_app\                    Flutter App（电脑调试工具）
│   ├─ watch_mp\                     微信小程序（手机端）
│   ├─ ota_server\                   OTA 服务器文件（version.txt）
│   └─ fw_release\               🚫 本机   固件发布包 .bin（用 U 盘/网盘拷）
│
├─ esp32s3_project\           ✅ 同步     自己手写的练习项目
├─ esp32s3_test\              ✅ 同步     测试工程
│
├─ 资料\                      🚫 本机     芯片资料/屏幕资料/参考工程/驱动
│
├─ sync.bat / sync.ps1                    一键同步脚本
├─ README.md / SETUP-笔记本.md
└─ 项目网址.txt
```

**规则**：

- 顶层文件夹默认全部同步；唯一例外是 `资料\`（体积大，本机专用）
- 工程文件夹里"人写的"文件全部同步；"机器生成的"（`build\`、`managed_components\`、`.dart_tool\`）和"密码文件"（`sdkconfig`）不上传——它们会自动重建或各机私有
- 笔记规则：总的笔记放最外层（比如本文件）；某个工程自己的笔记放工程里（比如 `watch\esp32s3_watch\项目说明.md`）

## 日常同步（唯一需要记住的事）

双击根目录的 **`sync.bat`**：自动「拉取 → 提交 → 推送」，只传变化的文件（通常几 KB）。

## 新电脑首次搭建

见 [SETUP-笔记本.md](SETUP-笔记本.md)。

## 给协作者开权限

GitHub 仓库页面 → **Settings → Collaborators → Add people**，仅被邀请的人可见。

## 注意

- WiFi 密码只写在 `sdkconfig`（已忽略）里；`sync.bat` 内置密码闸门。
- `*.bin` 固件发布包不入库（含密码明文，且每次编译都变）。
