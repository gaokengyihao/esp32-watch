/* page_manager.c —— 页面栈管理器实现 */
#include "page_manager.h"
#include "display.h"
#include "status_led.h"
#include "esp_log.h"
#include <stdbool.h>

/* 蓝牙升级中锁住按键操作（实现在 pages.c） */
bool pages_ui_locked(void);

#define STACK_DEPTH 8

/* 页面切换动画时长（毫秒）：越小越"跟手"，越大越"从容"
 *   200 = 默认，能看清整个滑动过程
 *   120 = 更快、更利索（现在用的）
 *   300+ = 慢悠悠（手表上会显得拖沓）
 * 想调就改这一个数字，重新编译烧录即可 */
#define PAGE_ANIM_MS  120

static const char *TAG = "page";

static page_t *s_stack[STACK_DEPTH];   /* 页面栈 */
static int s_depth;                    /* 栈里页面个数 */

static void load_page(page_t *p, lv_screen_load_anim_t anim)
{
    if (p->screen == NULL) {
        p->screen = p->create();       /* 懒加载：第一次进入才创建界面 */
    }

    lv_screen_load_anim(p->screen, anim, PAGE_ANIM_MS, 0, false);
    status_led_set(p->led_r, p->led_g, p->led_b);
    ESP_LOGI(TAG, "进入页面: %s (栈深度 %d)", p->name, s_depth);
}

void page_manager_init(page_t *root)
{
    if (!display_lock(1000)) {
        ESP_LOGE(TAG, "LVGL 加锁失败");
        return;
    }
    s_depth = 0;
    s_stack[s_depth++] = root;
    load_page(root, LV_SCREEN_LOAD_ANIM_NONE);
    display_unlock();
}

void page_manager_push(page_t *page)
{
    if (s_depth >= STACK_DEPTH) {
        ESP_LOGW(TAG, "页面栈已满");
        return;
    }
    if (!display_lock(1000)) {
        return;
    }
    s_stack[s_depth++] = page;
    load_page(page, LV_SCREEN_LOAD_ANIM_MOVE_LEFT);
    display_unlock();
}

void page_manager_pop(void)
{
    if (s_depth <= 1) {
        return;   /* 根页面不允许退出 */
    }
    if (!display_lock(1000)) {
        return;
    }
    s_depth--;
    load_page(s_stack[s_depth - 1], LV_SCREEN_LOAD_ANIM_MOVE_RIGHT);
    display_unlock();
}

page_t *page_manager_current(void)
{
    return (s_depth > 0) ? s_stack[s_depth - 1] : NULL;
}

int page_manager_depth(void)
{
    return s_depth;
}

void page_manager_handle_key(button_event_t ev)
{
    /* 蓝牙升级中：锁住所有按键操作（实现在 pages.c） */
    if (pages_ui_locked()) {
        ESP_LOGI(TAG, "蓝牙升级中：按键操作已锁定");
        return;
    }
    page_t *p = page_manager_current();
    if (p && p->on_key) {
        p->on_key(ev);
    }
}
