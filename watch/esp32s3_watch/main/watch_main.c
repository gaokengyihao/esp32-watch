/*
 * ============================================================================
 * ESP32-S3 智能手表 —— 主程序
 * ============================================================================
 * 项目目标：做出一个类似 OV-Watch 的智能手表（面试项目）
 *
 * 学习路线（每完成一步提交一次代码）：
 *   [x] 第1步：工程骨架 + 屏幕点亮 + LVGL 表盘 + 状态灯
 *   [x] 第2步：多页面（表盘/菜单/秒表/关于）+ 按键交互 + 页面栈
 *   [x] 第3步：触摸屏（CST816T，I2C）接入 LVGL
 *   [ ] 第4步：MPU6050 六轴传感器（I2C）：抬腕亮屏、计步
 *   [ ] 第5步：低功耗（light sleep + RTC 唤醒）
 *   [ ] 第6步：BLE 蓝牙（手机改表盘 / 收通知）
 *   [ ] 第7步：OTA 无线升级
 *
 * 硬件（YD-ESP32-S3 + P183B001-V4-CTP 1.83寸屏）：
 *   屏幕（SPI）：SCLK=12 MOSI=11 CS=10 DC=13 RST=14（背光 PWR 直接接 3V3）
 *   触摸（I2C）：SCL=9 SDA=8 RST=6 INT=7
 *
 * 按键操作说明（板载 BOOT 键）：
 *   表盘：短按进菜单 / 菜单：短按移光标，长按进入 / 秒表：短按开始暂停，长按清零返回
 * 状态灯颜色 = 当前页面（表盘蓝心跳 / 菜单绿 / 秒表黄 / 关于紫）
 * ============================================================================
 */
#include "esp_log.h"
#include "display.h"
#include "touch.h"
#include "ui.h"
#include "status_led.h"
#include "button.h"
#include "page_manager.h"
#include "wifi_time.h"
#include "power_mgr.h"
#include "ble_watch.h"
#include "watch_version.h"
#include "esp_ota_ops.h"

static const char *TAG = "watch";

/*
 * 演示模式：每 4 秒自动切换一次页面（没有按键/不触摸时也能看到界面在动）。
 * 现象：板载状态灯会按 蓝(表盘) -> 绿(菜单) -> 黄(秒表) -> 绿 -> 紫(关于) -> 绿 -> 蓝 循环变色。
 * 不需要时把下面这个宏改成 0。
 */
#define DEMO_AUTO_SWITCH 0

#if DEMO_AUTO_SWITCH
#include "pages.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void demo_task(void *arg)
{
    (void)arg;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(4000));
        page_manager_push(page_menu_get());          /* 表盘 -> 菜单 */
        vTaskDelay(pdMS_TO_TICKS(4000));
        page_manager_push(page_stopwatch_get());     /* 菜单 -> 秒表 */
        vTaskDelay(pdMS_TO_TICKS(4000));
        page_manager_pop();                          /* 秒表 -> 菜单 */
        vTaskDelay(pdMS_TO_TICKS(2000));
        page_manager_push(page_about_get());         /* 菜单 -> 关于 */
        vTaskDelay(pdMS_TO_TICKS(4000));
        page_manager_pop();                          /* 关于 -> 菜单 */
        vTaskDelay(pdMS_TO_TICKS(2000));
        page_manager_pop();                          /* 菜单 -> 表盘 */
    }
}
#endif

/* BOOT 键事件：睡醒第一下只当"点亮屏幕"，不执行操作 */
static void button_dispatch(button_event_t ev)
{
    if (power_mgr_is_waking()) {
        return;
    }
    power_mgr_notify_activity();
    page_manager_handle_key(ev);
}

void app_main(void)
{
    ESP_LOGI(TAG, "================================");
    ESP_LOGI(TAG, "ESP32-S3 智能手表启动（固件 v%s）", WATCH_FW_VERSION);
    ESP_LOGI(TAG, "================================");

    /* 如果是"OTA 升级后的第一次启动"：确认新固件可用。
     * 不确认的话，下次重启 bootloader 会认为新固件没通过验证，
     * 自动回滚到旧版本 —— 这是防止升级把表刷成砖的保险。 */
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (running != NULL &&
        esp_ota_get_state_partition(running, &ota_state) == ESP_OK &&
        ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
        esp_ota_mark_app_valid_cancel_rollback();
        ESP_LOGI(TAG, "OTA 新固件确认可用（已取消自动回滚）");
    }

    /* 1. 状态灯：开机亮红灯（表示"正在启动"） */
    ESP_ERROR_CHECK(status_led_init());
    status_led_set(32, 0, 0);

    /* 2. 屏幕 + LVGL */
    ESP_ERROR_CHECK(display_init());

    /* 3. 触摸屏（CST816T / I2C）→ 接入 LVGL 输入设备
     *    注意：故意不用 ESP_ERROR_CHECK —— 触摸没接好时只警告，不影响显示
     *    （否则 touch_init 失败会 abort → 芯片不停重启） */
    if (touch_init() != ESP_OK) {
        ESP_LOGE(TAG, "触摸初始化失败（检查接线：TSCL=9 TSDA=8 TRST=6 TINT=7）");
    }

    /* 4. 页面系统（默认显示表盘页） */
    ui_init();

    /* 4.5 联网对时（WiFi + NTP）：后台任务，不卡开机。
     *     没配 WiFi / 连不上时，表盘用固件编译时间兜底 */
    if (wifi_time_start() != ESP_OK) {
        ESP_LOGW(TAG, "对时任务启动失败（表盘用编译时间）");
    }

    /* 5. 按键：BOOT 键事件交给页面管理器分发（button_dispatch 里带睡醒保护） */
    ESP_ERROR_CHECK(button_init(button_dispatch));

    /* 6. 低功耗：15 秒无操作自动熄屏 + light sleep（触摸/BOOT/定时唤醒）
     *    注意：必须放在 button_init 之后 —— BOOT 键的 GPIO 要先配好，
     *    否则"按键唤醒"挂不上 */
    if (power_mgr_init() != ESP_OK) {
        ESP_LOGW(TAG, "低功耗初始化失败（不影响其它功能）");
    }

    /* 7. 蓝牙（BLE）：手机 App 看数据/版本 + 蓝牙传固件升级（OTA over BLE） */
    if (ble_watch_start() != ESP_OK) {
        ESP_LOGW(TAG, "蓝牙初始化失败（不影响其它功能）");
    }

#if DEMO_AUTO_SWITCH
    ESP_LOGW(TAG, "演示模式：每 4 秒自动切换页面（看状态灯颜色变化）");
    xTaskCreate(demo_task, "demo", 3072, NULL, 4, NULL);
#endif

    ESP_LOGI(TAG, "初始化完成：屏幕 + 触摸 + 按键 + 状态灯");
}
