/*
 * wifi_time.h —— WiFi + 联网对时（SNTP），以及"按需开/关 WiFi"
 *
 * 用法：
 *   app_main 里调 wifi_time_start()：后台连 WiFi + NTP 对时，完事自动关 WiFi
 *   界面里用 wifi_time_now() 取当前时间，wifi_time_is_synced() 看是否已对时
 *
 * 没配 WiFi / 连不上 / NTP 超时 时，会退回"固件编译时间"，
 * 表盘照样能显示一个接近的时间，不会空着或在 1970 年。
 *
 * OTA 等其它联网功能：用 wifi_time_wifi_on() / wifi_time_wifi_off() 按需开关。
 */
#pragma once

#include <stdbool.h>
#include <time.h>
#include "esp_err.h"

/* 开机时调一次：后台联网对时（不阻塞界面） */
esp_err_t wifi_time_start(void);

/* 是否已经通过 NTP 对时成功 */
bool wifi_time_is_synced(void);

/* 当前时间（同步前 = 固件编译时间 + 开机后经过的秒数） */
time_t wifi_time_now(void);

/* ---- 给 OTA 等联网功能用 ---- */

/* 打开 WiFi 并连接；timeout_ms 内没拿到 IP 就返回 ESP_FAIL */
esp_err_t wifi_time_wifi_on(uint32_t timeout_ms);

/* 关闭 WiFi（省电，也避免发射电流让屏幕背光闪） */
void wifi_time_wifi_off(void);

/* 现在是否连着 WiFi */
bool wifi_time_is_connected(void);
