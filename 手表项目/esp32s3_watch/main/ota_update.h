/*
 * ota_update.h —— 在线升级（OTA）：检查版本 + 确认升级
 *
 * 用法（界面里）：
 *   1) 用户点"CHECK UPDATE" -> ota_update_check_start()
 *      -> 后台去服务器拿 version.txt，和本机 WATCH_FW_VERSION 比
 *   2) ota_update_state():
 *        OTA_STATE_CHECKING      正在检查
 *        OTA_STATE_UP_TO_DATE    已是最新（界面不显示升级按钮）
 *        OTA_STATE_UPDATE_FOUND  发现新版本（界面显示"升级"按钮等用户确认）
 *        OTA_STATE_CHECK_FAILED  检查失败
 *   3) 用户确认 -> ota_update_start(url) -> 下载写另一分区 -> 重启
 *
 * 注意：升级过程中断网/断电都没关系 —— 新固件写的是另一块分区，
 *       失败还是跑老固件（这是 A/B 分区方案最大的好处）。
 */
#pragma once

#include <stdbool.h>
#include "esp_err.h"

typedef enum {
    OTA_STATE_IDLE = 0,       /* 没在做事 */
    OTA_STATE_CHECKING,       /* 正在检查服务器版本 */
    OTA_STATE_UP_TO_DATE,     /* 已是最新版本 */
    OTA_STATE_UPDATE_FOUND,   /* 发现新版本，等用户确认 */
    OTA_STATE_CHECK_FAILED,   /* 检查失败（没联网/服务器没 version.txt） */
    OTA_STATE_CONNECTING,     /* 用户已确认：正在连 WiFi */
    OTA_STATE_DOWNLOADING,    /* 正在下载 */
    OTA_STATE_DONE,           /* 成功，马上重启 */
    OTA_STATE_FAILED,         /* 升级失败（看 ota_update_message()） */
} ota_state_t;

/* 后台检查服务器上有没有新版本（需要联网） */
esp_err_t ota_update_check_start(void);

/* 开始升级（不阻塞；已在升级中会返回 ESP_ERR_INVALID_STATE）
 * 只应该在 ota_update_state() == OTA_STATE_UPDATE_FOUND 时调用 */
esp_err_t ota_update_start(const char *url);

ota_state_t ota_update_state(void);
int         ota_update_percent(void);
const char *ota_update_message(void);

/* 检查到的最新版本号（如 "1.4"）；没检查到就是空串 */
const char *ota_update_latest_version(void);
