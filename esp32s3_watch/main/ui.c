/*
 * ui.c —— 界面入口
 * 具体的页面都在 pages.c 里，这里只负责启动页面系统。
 */
#include "ui.h"
#include "page_manager.h"
#include "pages.h"
#include "esp_log.h"

static const char *TAG = "ui";

void ui_init(void)
{
    page_manager_init(page_watchface_get());
    pages_ble_ota_ui_init();     /* 蓝牙升级进度页的轮询定时器（升级时自动切页+锁操作） */
    ESP_LOGI(TAG, "UI 就绪（表盘页）");
}
