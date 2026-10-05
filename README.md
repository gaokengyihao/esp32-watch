# ESP32-S3 智能手表（学习 / 求职项目）

一个参考 OV-Watch 功能设计的智能手表：ESP32-S3 + ESP-IDF + LVGL，
手机端用微信小程序通过 BLE 连接手表（看数据、蓝牙传固件升级）。

## 目录结构

| 目录 | 内容 |
|---|---|
| **手表项目/** | 手表整套（AI 协助开发） |
| ├ `esp32s3_watch/` | 手表固件（ESP-IDF 工程）★ 核心；必读 `项目说明.md` |
| ├ `watch_mp/` | 手机端微信小程序（数据 / 升级 / 关于） |
| ├ `watch_app/` | Flutter 调试工具（可选） |
| ├ `ota_server/` | WiFi / 小程序升级用的 HTTP 服务器目录 |
| └ `fw_release/` | 固件版本存档（发布时自动更新） |
| 我的练习/ | 自己写的练习工程（`esp32s3_project`） |
| 测试/ | 测试工程（`esp32s3_test`） |
| 资料/ | 芯片 / 屏幕资料（`ESP32-S3资料`、`TFT屏资料`） |
| 参考例程/ | OV-Watch 开源参考项目 + `项目网址.txt` |
| 工具/ | 板子 USB 驱动（`drivers/usb_jtag_driver`，备用） |

## 换机器开发注意

1. `git clone` 本仓库（**不含**：编译产物、managed_components、sdkconfig、参考资料大文件）
2. 第一次编译：`idf.py build` 会重新生成 `sdkconfig` 并自动下载组件
3. WiFi 配置（对时/OTA 用）：`idf.py menuconfig -> Watch 手表配置`（不提交到仓库）
4. **AI 协作上下文**：开工前让 AI 先读 `手表项目/esp32s3_watch/项目说明.md`

## 当前版本

- 固件 `v2.9`：屏幕/触摸/页面栈/低功耗（5秒睡5秒蓝牙广播窗口）/BLE OTA（~7 KB/s）/A-B 分区回滚
- 小程序：连接看数据、一键升级（自动检查版本 → 下载 → 蓝牙传输 → 自动确认版本）
