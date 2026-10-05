/* touch.h —— 触摸屏（CST816T / I2C）接口 */
#pragma once

#include "esp_err.h"

/* 初始化触摸屏并接入 LVGL。必须在 display_init() 成功之后调用 */
esp_err_t touch_init(void);
