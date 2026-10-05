/*
 * status_led.c —— 板载 RGB 灯（WS2812）驱动
 *
 * 用 RMT 外设模拟 WS2812 的单线协议（800kHz），这是 ESP32 上最标准的做法。
 * 手表里它的角色：系统状态指示 / 消息提醒灯。
 *
 * 注意：GPIO48 和 GPIO38 两个脚同时输出，哪个亮说明你的板子用哪个。
 */
#include "status_led.h"
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"

#define LED_GPIO_A         48
#define LED_GPIO_B         38

#define RMT_RESOLUTION_HZ  (10 * 1000 * 1000)   /* 1 tick = 0.1us */

#define WS2812_T0H  3   /* 比特0：高 0.3us */
#define WS2812_T0L  9   /* 比特0：低 0.9us */
#define WS2812_T1H  6   /* 比特1：高 0.6us */
#define WS2812_T1L  6   /* 比特1：低 0.6us */

static const char *TAG = "led";

static rmt_channel_handle_t s_chan[2];
static rmt_encoder_handle_t s_encoder;
static rmt_symbol_word_t s_symbols[25];      /* 24bit 颜色 + 1 个复位符号 */
static SemaphoreHandle_t s_mutex;

static void led_write(uint8_t r, uint8_t g, uint8_t b)
{
    /* WS2812 的字节顺序是 G-R-B，高位先发 */
    uint32_t grb = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;

    for (int bit = 0; bit < 24; bit++) {
        bool one = (grb >> (23 - bit)) & 1;
        s_symbols[bit].level0 = 1;
        s_symbols[bit].duration0 = one ? WS2812_T1H : WS2812_T0H;
        s_symbols[bit].level1 = 0;
        s_symbols[bit].duration1 = one ? WS2812_T1L : WS2812_T0L;
    }

    /* 结束后拉低 >50us 作为复位信号 */
    s_symbols[24].level0 = 0;
    s_symbols[24].duration0 = 750;   /* 75us */
    s_symbols[24].level1 = 0;
    s_symbols[24].duration1 = 0;

    rmt_transmit_config_t tx_cfg = { .loop_count = 0 };
    for (int i = 0; i < 2; i++) {
        ESP_ERROR_CHECK(rmt_transmit(s_chan[i], s_encoder, s_symbols,
                                     sizeof(s_symbols), &tx_cfg));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(s_chan[i], portMAX_DELAY));
    }
}

esp_err_t status_led_init(void)
{
    const int gpios[2] = { LED_GPIO_A, LED_GPIO_B };

    for (int i = 0; i < 2; i++) {
        rmt_tx_channel_config_t chan_cfg = {
            .clk_src = RMT_CLK_SRC_DEFAULT,
            .gpio_num = gpios[i],
            .mem_block_symbols = 48,
            .resolution_hz = RMT_RESOLUTION_HZ,
            .trans_queue_depth = 4,
        };
        esp_err_t err = rmt_new_tx_channel(&chan_cfg, &s_chan[i]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "RMT 通道 %d 创建失败", i);
            return err;
        }
        ESP_ERROR_CHECK(rmt_enable(s_chan[i]));
    }

    rmt_copy_encoder_config_t enc_cfg = {};
    esp_err_t err = rmt_new_copy_encoder(&enc_cfg, &s_encoder);
    if (err != ESP_OK) {
        return err;
    }

    s_mutex = xSemaphoreCreateMutex();
    if (s_mutex == NULL) {
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "板载 RGB 灯就绪");
    return ESP_OK;
}

void status_led_set(uint8_t r, uint8_t g, uint8_t b)
{
    if (s_mutex == NULL) {
        return;
    }
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        led_write(r, g, b);
        xSemaphoreGive(s_mutex);
    }
}
