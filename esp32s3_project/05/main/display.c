/*
 * display.c —— 屏幕 + LVGL 初始化
 *
 * 屏幕型号：1.83 寸 240x284 SPI 触摸屏
 *           P183B001-V4-CTP（显示驱动 ST7789P3，触摸 CST816T，4线 SPI）
 * 说明：ST7789 系列内部显存是 240x320，面板只用了其中一部分，
 *       所以 "分辨率 + Y偏移(gap)" 必须和实际面板一致，否则画面错位/顶部花屏。
 *       本屏（240x284）厂商例程参数：分辨率 240x284，X/Y 偏移都是 0。
 *
 * ===== 接线（按你的实际接线修改这几个宏）=====
 *   LCD_SCLK  GPIO12
 *   LCD_MOSI  GPIO11
 *   LCD_CS    GPIO10
 *   LCD_DC    GPIO13
 *   LCD_RST   GPIO14
 *   LCD_BL    -1      （模块没有背光控制脚：PWR 直接接 3V3，背光常亮）
 */
#include "display.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LCD_HOST      SPI2_HOST
#define PIN_LCD_SCLK  12
#define PIN_LCD_MOSI  11
#define PIN_LCD_CS    10
#define PIN_LCD_DC    13
#define PIN_LCD_RST   14
#define PIN_LCD_BL    -1     /* 本模块无背光控制脚（背光常亮） */

/* 分辨率宏在 display.h 里定义（LCD_H_RES / LCD_V_RES），触摸驱动也要用 */

static const char *TAG = "display";
static lv_display_t *s_lv_disp;
static esp_lcd_panel_handle_t s_panel;   /* 面板句柄：熄屏/亮屏要用 */

esp_err_t display_init(void)
{
    /* 1. 打开背光（本模块没有背光控制脚：PIN_LCD_BL 为 -1 时整段不编译） */
#if PIN_LCD_BL >= 0
    gpio_config_t bl_cfg = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_LCD_BL,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&bl_cfg), TAG, "backlight gpio");
    gpio_set_level(PIN_LCD_BL, 1);
#endif

    /* 2. 初始化 SPI 总线 */
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_LCD_SCLK,
        .mosi_io_num = PIN_LCD_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO), TAG, "spi bus");

    /* 3. 屏幕控制接口（负责发命令和数据） */
    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = PIN_LCD_CS,
        .dc_gpio_num = PIN_LCD_DC,
        .spi_mode = 0,
        .pclk_hz = 80 * 1000 * 1000,   /* SPI 时钟 80MHz（原来 40MHz）：刷屏快一倍，更流畅
                                        * 如果画面出现花屏/噪点，说明信号质量不行，改回 60MHz 或 40MHz */
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_cfg, &io),
                        TAG, "panel io");

    /* 4. 创建 ST7789 屏幕对象并初始化 */
    esp_lcd_panel_handle_t panel = NULL;
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(io, &panel_cfg, &panel), TAG, "st7789");

    esp_lcd_panel_reset(panel);
    esp_lcd_panel_init(panel);
    esp_lcd_panel_invert_color(panel, true);      /* IPS 屏，厂商初始化序列里有 0x21(INVON) */
    esp_lcd_panel_set_gap(panel, 0, 0);           /* 240x284 屏：无偏移（厂商例程 Y_OFFSET=0） */
    esp_lcd_panel_disp_on_off(panel, true);

    /* 5. 把屏幕挂到 LVGL（由 esp_lvgl_port 组件完成刷屏、tick 等杂活） */
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "lvgl port");

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io,
        .panel_handle = panel,
        .buffer_size = LCD_H_RES * 60,   /* 60 行像素的绘制缓冲（越大刷屏批次越少，越顺滑） */
        .double_buffer = true,           /* 双缓冲：一块在画、一块在发，界面更流畅 */
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = false,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
            .swap_bytes = true,
        },
    };
    s_lv_disp = lvgl_port_add_disp(&disp_cfg);
    if (s_lv_disp == NULL) {
        return ESP_FAIL;
    }

    s_panel = panel;
    ESP_LOGI(TAG, "LCD 初始化完成: %dx%d", LCD_H_RES, LCD_V_RES);
    return ESP_OK;
}

bool display_set_on(bool on)
{
    if (s_panel == NULL) {
        return false;
    }

    /* 注意：这里故意**不用** disp_sleep（面板睡眠命令）——
     * 实测 ST7789 进 sleep 后显存内容会丢，醒来是一片白，
     * 而 LVGL 不知道要重画。只关"显示输出"就够了（显存还在）。 */
    if (on) {
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));
    } else {
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, false));
    }
    ESP_LOGI(TAG, "屏幕%s", on ? "点亮" : "熄灭");
    return true;
}

void display_reinit(void)
{
    if (s_panel == NULL) {
        return;
    }
    /* 重新发一遍厂商初始化序列 + 我们额外设置的东西（反色/偏移），
     * 用于"睡醒后救回被扰动的面板" */
    esp_lcd_panel_init(s_panel);
    esp_lcd_panel_invert_color(s_panel, true);
    esp_lcd_panel_set_gap(s_panel, 0, 0);
    ESP_LOGI(TAG, "屏幕已重新初始化");
}

bool display_lock(uint32_t timeout_ms)
{
    return lvgl_port_lock(timeout_ms);
}

void display_unlock(void)
{
    lvgl_port_unlock();
}

lv_display_t *display_get(void)
{
    return s_lv_disp;
}
