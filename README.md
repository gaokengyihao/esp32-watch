# ESP32-S3 智能手表（学习 / 求职项目）

一个参考 OV-Watch 功能设计的智能手表：ESP32-S3 + ESP-IDF + LVGL，
手机端用微信小程序通过 BLE 连接手表（看数据、蓝牙传固件升级）。

## 目录结构

| 目录 | 内容 |
|---|---|
| **esp32s3_watch/** | 手表固件（ESP-IDF 工程）★ 主要开发在这里 |
| | → 必读：`项目说明.md`（学习路线 / 接线 / 踩坑记录 / 蓝牙协议 / 面试讲点） |
| **watch_mp/** | 手机端：微信小程序（数据 / 升级 / 关于） |
| | → 说明：`README.md`（导入方式 / 蓝牙 UUID / 发布流程） |
| watch_app/ | Flutter 调试工具（可选） |
| ota_server/ | WiFi-OTA 的 HTTP 服务器目录（version.txt + 固件，bin 不进仓库） |
| fw_release/ | 发布的固件存档（自动生成） |
| drivers/ | 板子 USB 驱动 |
| esp32s3_test/ | 早期练手工程 |

## 换机器开发注意

1. `git clone` 本仓库（**不含**：编译产物、managed_components、sdkconfig、参考资料大文件）
2. 第一次编译：`idf.py build` 会重新生成 `sdkconfig` 并自动下载组件
3. WiFi 配置（对时/OTA 用）：`idf.py menuconfig -> Watch 手表配置`（不提交到仓库）
4. **AI 协作上下文**：开工前让 AI 先读 `esp32s3_watch/项目说明.md`（所有进度和踩坑都在里面）

## 当前版本

- 固件 `v2.9`：屏幕/触摸/页面栈/低功耗（5秒睡5秒蓝牙广播窗口）/BLE OTA（~7 KB/s）/A-B 分区回滚
- 小程序：连接看数据、一键升级（自动检查版本 → 下载 → 蓝牙传输 → 自动确认版本）
