/* button.h —— 板载 BOOT 按键驱动（GPIO0） */
#pragma once

#include "esp_err.h"

typedef enum {
    BTN_EVENT_SHORT_PRESS = 0,   /* 短按（松手时触发） */
    BTN_EVENT_LONG_PRESS,        /* 长按（按住 1 秒时触发一次） */
} button_event_t;

typedef void (*button_cb_t)(button_event_t ev);

/*
 * 初始化按键。事件通过回调上报。
 * 注意：回调运行在按键任务里，别在里面做耗时操作（要操作 LVGL 记得加锁）。
 */
esp_err_t button_init(button_cb_t cb);
