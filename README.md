# ESP32-S3 智能手表（个人开发仓库）

固件 + Flutter App + 微信小程序 + OTA 文件，**公司电脑 ↔ 宿舍笔记本**通过本仓库同步。

## 日常同步（唯一需要记住的事）

双击根目录的 **`sync.bat`** 即可：自动「拉取 → 提交 → 推送」一条龙。

> 只会传输有改动的文件（通常几 KB ~ 几十 KB），不会全量上传。

## 目录结构

| 目录 | 内容 |
|---|---|
| `esp32s3_watch/` | ESP32-S3 固件（ESP-IDF 工程） |
| `watch_app/` | Flutter App |
| `watch_mp/` | 微信小程序 |
| `ota_server/` | OTA 版本文件（version.txt） |
| `fw_release/` | 发布记录（.bin 不入库，见下） |

## 不会同步的内容（各机器本地生成 / 保留）

- `build/`、`managed_components/`、`.dart_tool/` —— 编译产物，`idf.py build` / `flutter pub get` 会自动重建
- `sdkconfig` —— **含 WiFi 密码，各机私有**；新机器第一次用 `idf.py menuconfig -> Watch 手表配置` 填自己的 WiFi
- `*.bin` 固件发布包 —— 每次编译都变且含密码明文，用 U 盘/网盘单独拷
- `ESP32-S3资料/`、`TFT屏资料/`、`OV-Watch-参考/` 等参考资料

## 新电脑首次搭建

见 [SETUP-笔记本.md](SETUP-笔记本.md)。

## 给协作者开权限

GitHub 仓库页面 → **Settings → Collaborators → Add people**，邀请后对方即可查看/推送（仅被邀请的人可见）。

## 注意

- 不要往被跟踪的文件里写密码/密钥；WiFi 密码只写在 `sdkconfig`（已忽略）里。
- `sync.bat` 内置了密码闸门：一旦暂存区出现非空的 `CONFIG_WATCH_WIFI_PASSWORD`，会拒绝提交。
