/*
 * button.c —— 板载 BOOT 按键（GPIO0）
 *
 * 用"轮询 + 状态机"实现，比中断更好理解，还能天然去抖：
 *   每 10ms 读一次电平
 *   按下 -> 记录时间
 *   按住超过 1 秒 -> 发"长按"事件（只发一次）
 *   松手 -> 如果没发过长按，就发"短按"事件
 */
#include "button.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PIN_BUTTON      0       /* 板载 BOOT 键 */
#define POLL_MS         10      /* 轮询周期 */
#define MIN_PRESS_MS    50      /* 短于 50ms 视为抖动，忽略 */
#define LONG_PRESS_MS   1000    /* 按住超过 1 秒 = 长按 */

static const char *TAG = "button";
static button_cb_t s_cb;

static void button_task(void *arg)
{
    (void)arg;
    bool pressed_last = false;
    int64_t press_time = 0;
    bool long_sent = false;

    while (1) {
        bool pressed = (gpio_get_level(PIN_BUTTON) == 0);   /* 按下为低电平 */
        int64_t now = esp_timer_get_time();

        if (pressed && !pressed_last) {
            /* 刚按下 */
            press_time = now;
            long_sent = false;
        } else if (!pressed && pressed_last) {
            /* 刚松手 */
            int64_t dur_ms = (now - press_time) / 1000;
            if (!long_sent && dur_ms >= MIN_PRESS_MS) {
                if (s_cb) {
                    s_cb(BTN_EVENT_SHORT_PRESS);
                }
            }
        } else if (pressed && !long_sent) {
            /* 按住中 */
            if ((now - press_time) / 1000 >= LONG_PRESS_MS) {
                long_sent = true;
                if (s_cb) {
                    s_cb(BTN_EVENT_LONG_PRESS);
                }
            }
        }

        pressed_last = pressed;
        vTaskDelay(pdMS_TO_TICKS(POLL_MS));
    }
}

esp_err_t button_init(button_cb_t cb)
{
    s_cb = cb;

    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << PIN_BUTTON,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPIO 初始化失败");
        return err;
    }

    if (xTaskCreate(button_task, "button", 3072, NULL, 5, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "按键就绪（BOOT 键 / GPIO0）");
    return ESP_OK;
}
