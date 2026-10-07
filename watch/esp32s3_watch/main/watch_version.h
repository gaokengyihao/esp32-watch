/*
 * watch_version.h —— 固件版本号
 *
 * 做 OTA 演示的时候：
 *   1) 把下面那行的版本号改成新的（比如 1.7），重新编译
 *   2) 双击 tools\publish.bat 发布（会自动复制到 D:\esp32\watch\fw_release 并删掉旧的）
 *   3) 手机小程序升级页会自动发现新版本 -> 蓝牙升级
 *   4) 手表的"关于"页也看得到版本号变化
 */
#pragma once

#define WATCH_FW_VERSION  "2.4"
