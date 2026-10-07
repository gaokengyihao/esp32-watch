/*
 * ble_watch.h —— 手表蓝牙（BLE 5）服务：手机 App 看数据 + 蓝牙传固件升级
 *
 * 为什么用 BLE 不用经典蓝牙：
 *   - ESP32-S3 只有 BLE（低功耗），手机手表行业用的也都是 BLE
 *   - 经典蓝牙(BR/EDR)更费电，手机这边也不方便
 *
 * 对外提供 1 个服务（UUID 0xFF00），4 个特征值：
 *   0xFF01  读        固件版本（"1.6"）               <- App 看版本
 *   0xFF02  读 + 通知  状态数据（"fw=1.6;up=123s;..."） <- App 看数据/看升级进度
 *   0xFF03  写        命令（测试用："reboot"）          <- App 发指令
 *   0xFF04  写        固件数据流（OTA）                <- App 把新固件传过来
 *
 * 【OTA 协议】手机端照这个实现即可（脚本 tools/ble_ota.py 就是例子）：
 *   1) 先往 0xFF04 写 4 字节小端 = 固件总大小（带响应写）
 *   2) 然后把 build/esp32s3_watch.bin 按块写进 0xFF04（每块建议 244 字节，无响应写）
 *   3) 手表收满总大小 -> 校验 -> 写进"另一块"分区 -> 重启进新固件
 *      过程中 0xFF02 会通知 "ota=45%" 这样的进度
 *   4) 传一半断线/断电也不怕：复用的还是 A/B 双分区 + 回滚保护
 */
#pragma once

#include "esp_err.h"

/* 启动 BLE（初始化 NimBLE + 注册服务 + 开始广播） */
esp_err_t ble_watch_start(void);

/* 睡醒后调用：light sleep 会把射频（含广播）停掉，这里负责重新拉起来 */
void ble_watch_on_wake(void);
