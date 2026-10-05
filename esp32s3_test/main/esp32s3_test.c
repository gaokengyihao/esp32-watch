/*
 * ============================================================================
 * ESP32-S3 第一个程序：串口打印 + 板载 RGB 灯(WS2812)闪烁
 * ============================================================================
 * 给从 STM32 + Keil 转过来的朋友的说明：
 *
 * 1) ESP32 不用 Keil，用官方 SDK "ESP-IDF"，在命令行(或 VS Code)里开发：
 *      改代码 ===> idf.py build ===> idf.py -p COMx flash ===> idf.py -p COMx monitor
 *      写代码        编译成 bin         用 USB 线下载到芯片           看串口打印
 *
 * 2) ESP32-S3 芯片自带 USB 串口/JTAG，或者开发板上焊了 USB 转串口芯片(CH340/CP210x)，
 *    插一根 USB 线就能下载程序，不需要 J-Link/ST-Link 那类下载器。
 *
 * 3) 如果自动下载失败：按住 BOOT 键不放 -> 点一下 RST 键 -> 松开 BOOT，
 *    芯片就进入"下载模式"，再重新执行 flash 命令即可。
 *
 * 4) 退出串口监视：按 Ctrl + ]
 * ============================================================================
 */

#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "driver/rmt_tx.h"

static const char *TAG = "app";

/* ---------------- 板载 RGB 灯 (WS2812) 驱动 ---------------- *
 * 板载 RGB 灯常见 GPIO：
 *   ESP32-S3-DevKitC-1 v1.0 ................ GPIO48
 *   ESP32-S3-DevKitC-1 v1.1 / 多数 N16R8 .... GPIO38
 * 本程序同时驱动这两个引脚，哪个灯亮说明你的板子用哪个，对板子没有副作用。
 */
#define LED_GPIO_A 48
#define LED_GPIO_B 38

#define RMT_RESOLUTION_HZ (10 * 1000 * 1000) /* RMT 计时: 1 tick = 0.1us */

#define WS2812_T0H 3 /* 比特0: 高电平 0.3us */
#define WS2812_T0L 9 /* 比特0: 低电平 0.9us */
#define WS2812_T1H 6 /* 比特1: 高电平 0.6us */
#define WS2812_T1L 6 /* 比特1: 低电平 0.6us */

static rmt_channel_handle_t s_led_chan[2];
static rmt_encoder_handle_t s_copy_encoder;
static rmt_symbol_word_t s_led_symbols[25]; /* 24bit 颜色数据 + 1 个复位符号 */

static void led_init(void)
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
        ESP_ERROR_CHECK(rmt_new_tx_channel(&chan_cfg, &s_led_chan[i]));
        ESP_ERROR_CHECK(rmt_enable(s_led_chan[i]));
    }

    rmt_copy_encoder_config_t enc_cfg = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&enc_cfg, &s_copy_encoder));
}

static void led_set_color(uint8_t r, uint8_t g, uint8_t b)
{
    /* WS2812 的字节顺序是 G-R-B，高位先发 */
    uint32_t grb = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;

    for (int bit = 0; bit < 24; bit++) {
        bool one = (grb >> (23 - bit)) & 1;
        s_led_symbols[bit].level0 = 1;
        s_led_symbols[bit].duration0 = one ? WS2812_T1H : WS2812_T0H;
        s_led_symbols[bit].level1 = 0;
        s_led_symbols[bit].duration1 = one ? WS2812_T1L : WS2812_T0L;
    }

    /* 数据发完后保持低电平 >50us，作为复位/刷新信号 */
    s_led_symbols[24].level0 = 0;
    s_led_symbols[24].duration0 = 750; /* 75us */
    s_led_symbols[24].level1 = 0;
    s_led_symbols[24].duration1 = 0;

    rmt_transmit_config_t tx_cfg = { .loop_count = 0 };
    for (int i = 0; i < 2; i++) {
        ESP_ERROR_CHECK(rmt_transmit(s_led_chan[i], s_copy_encoder, s_led_symbols,
                                     sizeof(s_led_symbols), &tx_cfg));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(s_led_chan[i], portMAX_DELAY));
    }
}

/* ---------------- 主程序 ---------------- */

void app_main(void)
{
    esp_chip_info_t chip;
    esp_chip_info(&chip);

    const char *model;
    switch (chip.model) {
    case CHIP_ESP32S3: model = "ESP32-S3"; break;
    case CHIP_ESP32S2: model = "ESP32-S2"; break;
    case CHIP_ESP32:   model = "ESP32";    break;
    default:           model = "Unknown";  break;
    }

    /* 日志一律用英文：Windows 控制台默认是 GBK 编码，打中文会乱码 */
    ESP_LOGI(TAG, "==============================");
    ESP_LOGI(TAG, "Hello ESP32! chip: %s, %d cores, radio: %s%s", model, chip.cores,
             (chip.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi " : "",
             (chip.features & CHIP_FEATURE_BT) ? "BT" : "");
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());

    uint32_t flash_size = 0;
    if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
        ESP_LOGI(TAG, "Flash size: %" PRIu32 " MB", flash_size / (1024 * 1024));
    }

    size_t psram_size = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    if (psram_size > 0) {
        ESP_LOGI(TAG, "PSRAM size: %u KB", (unsigned)(psram_size / 1024));
    } else {
        ESP_LOGI(TAG, "PSRAM: not enabled");
    }

    ESP_LOGI(TAG, "Free heap: %u KB", (unsigned)(esp_get_free_heap_size() / 1024));
    ESP_LOGI(TAG, "==============================");

    led_init();

    const uint8_t colors[][3] = {
        { 32, 0, 0 }, /* 红 */
        { 0, 32, 0 }, /* 绿 */
        { 0, 0, 32 }, /* 蓝 */
        { 0, 0, 0 },  /* 灭 */
    };
    const char *color_names[] = { "RED", "GREEN", "BLUE", "OFF" };

    int seconds = 0;
    while (1) {
        int idx = seconds % 4;
        led_set_color(colors[idx][0], colors[idx][1], colors[idx][2]);
        ESP_LOGI(TAG, "RGB color: %s  (uptime %d s)", color_names[idx], seconds);
        seconds++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
