/*
 * page_manager.h —— 页面栈管理器
 *
 * 面试考点：界面多了以后怎么管理？
 * 用一个"栈"记录页面历史：
 *   进入新页面 -> push（入栈），动画从左滑入
 *   返回上一页 -> pop（出栈），动画向右滑出
 * 这就是 OV-Watch 里 ScrRenewStack 的做法（见它的 README 第 4 节）。
 */
#pragma once

#include "lvgl.h"
#include "button.h"

typedef struct page_s {
    const char *name;                   /* 页面名（日志用） */
    lv_obj_t *(*create)(void);          /* 创建界面（懒加载，只调用一次） */
    void (*on_key)(button_event_t ev);  /* 按键回调，可为 NULL */
    uint8_t led_r, led_g, led_b;        /* 进入本页时状态灯的颜色 */
    lv_obj_t *screen;                   /* 内部使用：创建好的界面 */
} page_t;

/* 初始化页面栈，并显示根页面 */
void page_manager_init(page_t *root);

/* 进入新页面（入栈） */
void page_manager_push(page_t *page);

/* 返回上一页（出栈）；根页面不会退出 */
void page_manager_pop(void);

/* 当前页面 */
page_t *page_manager_current(void);

/* 当前页面栈深度（1 = 只有根页面） */
int page_manager_depth(void);

/* 按键分发：把事件转给当前页面的 on_key（直接作为 button_init 的回调用） */
void page_manager_handle_key(button_event_t ev);
