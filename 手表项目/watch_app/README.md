# ESP32 手表 App（Flutter）

配套固件工程：`E:\esp32\project\手表项目\esp32s3_watch`（手表端蓝牙服务在 `main/ble_watch.c`）

## 先跑起来看界面（不需要蓝牙、不需要装大工具链）

```powershell
# 1. 装 Flutter SDK（约 1GB，一次性）
#    下载：https://docs.flutter.dev/get-started/install/windows
#    解压到 D:\flutter，然后把 D:\flutter\bin 加进 PATH
flutter doctor

# 2. 跑成网页（用 Chrome 看，最快，不需要 Visual Studio）
cd E:\esp32\project\手表项目\watch_app
flutter pub get
flutter run -d chrome

# 想跑成 Windows 桌面窗口也行（需要装 Visual Studio 的 C++ 桌面组件）：
flutter run -d windows
```

打开后默认是 **模拟模式**（右上角开关），界面里的数据是假的，
可以先把三个页面、交互、进度条都调顺；等蓝牙 dongle 到了，
把"模拟"开关关掉，就是真连手表。

## 三个页面

| 页面 | 内容 |
|---|---|
| 数据 | 步数 / 电量 / 固件版本 / 运行时间 / 连接按钮 / 日志 |
| 升级 | 显示手表版本 → 选固件文件 → 蓝牙传过去（进度条） |
| 关于 | 项目介绍 + 蓝牙 UUID 对照表 |

## 蓝牙对接（对应固件 `ble_watch.c`）

```
0xFF00  服务
0xFF01  读        固件版本        "1.6"
0xFF02  读 + 通知  状态数据        "fw=1.6;up=123s;step=0;bat=--"
0xFF03  写        命令           "reboot"
0xFF04  写        固件数据（OTA）  先写 4 字节小端总长度，再按 244 字节/块传
```

## 以后出手机 App

- **Android**：`flutter build apk`（不需要 Mac）
- **iOS**：需要 macOS + Xcode（可以借 Mac 或用云编译：Codemagic / GitHub Actions macOS runner）
- 手机端"自动检查新版本"的思路：App 联网拿云端版本号 → 和手表版本比 → 提示升级 → 把固件通过蓝牙传给手表
