/* pages.h —— 各个页面的获取接口 */
#pragma once

#include <stdbool.h>
#include "page_manager.h"

page_t *page_watchface_get(void);   /* 表盘 */
page_t *page_menu_get(void);        /* 菜单 */
page_t *page_stopwatch_get(void);   /* 秒表 */
page_t *page_about_get(void);       /* 关于 */
page_t *page_ota_get(void);         /* OTA 在线升级（进度页，手机蓝牙升级时备用） */

/* ---- 蓝牙升级（手机小程序推送）时的界面联动：显示进度 + 锁住所有操作 ---- */
bool pages_ui_locked(void);             /* true = 升级中，所有操作都不响应 */
void pages_ble_ota_show(void);          /* 开始升级：稍后自动切到进度页 */
void pages_ble_ota_progress(int pct);   /* 更新百分比（0~100） */
void pages_ble_ota_done(void);          /* 成功（马上要重启了） */
void pages_ble_ota_fail(void);          /* 失败/中断：显示几秒后自动返回 */
void pages_ble_ota_ui_init(void);       /* 启动进度页轮询定时器（ui_init 里调用） */
