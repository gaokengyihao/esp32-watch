/* display.h —— 屏幕 + LVGL 初始化接口 */
#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "lvgl.h"

/* 屏幕分辨率（触摸驱动的坐标范围也用这个） */
#define LCD_H_RES  240
#define LCD_V_RES  284

/* 初始化 SPI 屏幕并把 LVGL 挂上去。成功后才能调用 ui_init() */
esp_err_t display_init(void);

/* 访问 LVGL 前必须加锁（LVGL 不是线程安全的，别的任务在画界面时要锁住） */
bool display_lock(uint32_t timeout_ms);
void display_unlock(void);

/* 拿到 LVGL 的显示对象（一般用不到） */
lv_display_t *display_get(void);

/* 屏幕开/关（低功耗熄屏用）：false = 黑屏，true = 恢复点亮 */
bool display_set_on(bool on);

/* 重新初始化面板（睡醒后救回被扰动/复位的 ST7789 用） */
void display_reinit(void);
