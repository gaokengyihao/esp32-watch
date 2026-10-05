/*
 * touch.c —— 触摸屏驱动（CST816T，I2C）+ 接入 LVGL
 *
 * 屏幕模块：P183B001-V4-CTP（显示 ST7789P3，触摸 CST816T）
 * CST816T 和 CST816S 寄存器兼容，用乐鑫官方组件 espressif/esp_lcd_touch_cst816s
 *
 * ===== 接线 =====
 *   TP_SCL  GPIO9
 *   TP_SDA  GPIO8
 *   TP_RST  GPIO6
 *   TP_INT  GPIO7
 *
 * 说明：坐标读取由 LVGL 任务轮询（touch.c 自己注册的读回调，带 I2C 容错）。
 *       INT 脚不是必须的（接上也无妨，低功耗靠它唤醒）。
 */
#include "touch.h"
#include "display.h"
#include "power_mgr.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_cst816s.h"
#include "esp_lvgl_port.h"

#define PIN_TP_SCL   9
#define PIN_TP_SDA   8
#define PIN_TP_RST   6
#define PIN_TP_INT   7

static const char *TAG = "touch";

static esp_lcd_touch_handle_t s_touch;
static lv_indev_t *s_indev;

/* 触摸按下时打印坐标（调试用；手势判断在 pages.c 的屏幕事件里做） */
static void touch_event_cb(lv_event_t *e)
{
    if (s_indev == NULL) {
        return;
    }
    power_mgr_notify_activity();     /* 有触摸 = 有活动（低功耗计时用） */
    lv_point_t p;
    lv_indev_get_point(s_indev, &p);
    ESP_LOGI(TAG, "触摸: x=%d y=%d", p.x, p.y);
}

/* ============================================================================
 * 【为什么不用 esp_lvgl_port 的 lvgl_port_add_touch】
 * 乐鑫组件 esp_lvgl_port 的触摸任务里写死了：
 *     ESP_ERROR_CHECK(esp_lcd_touch_read_data(...));
 * 触摸 I2C 只要偶发一次读失败（总线抖动/芯片忙/静电），整个程序就会 abort 重启。
 * 实测踩坑：正连着蓝牙时摸一下屏幕，I2C 抖动 -> 手表崩溃重启 -> 手机端表现为"连接失败"。
 * 所以我们自己注册 LVGL 读回调：读失败就当作"当前没触摸"，最多丢一帧坐标，绝不崩溃。
 * ==========================================================================*/
static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    if (s_touch == NULL) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    /* 读失败：容错，当作松手，下一帧再试 */
    if (esp_lcd_touch_read_data(s_touch) != ESP_OK) {
        static uint32_t s_err_cnt;
        if ((s_err_cnt++ & 0x1F) == 0) {       /* 每 32 次才打一条，避免刷屏 */
            ESP_LOGW(TAG, "触摸 I2C 读失败（已容错，不重启），累计 %lu 次",
                     (unsigned long)(s_err_cnt));
        }
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    uint16_t x = 0, y = 0;
    uint8_t point_cnt = 0;
    if (esp_lcd_touch_get_coordinates(s_touch, &x, &y, NULL, &point_cnt, 1) && point_cnt > 0) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

esp_err_t touch_init(void)
{
    /* 1. 初始化 I2C 主机总线 */
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = PIN_TP_SDA,
        .scl_io_num = PIN_TP_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus = NULL;
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &bus), TAG, "i2c bus");

    /* 2. 给触摸芯片创建一个 LCD IO 句柄（走 I2C） */
    esp_lcd_panel_io_handle_t tp_io = NULL;
    esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(bus, &io_cfg, &tp_io), TAG, "touch io");

    /* 3. 创建 CST816S/T 触摸驱动 */
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = PIN_TP_RST,
        .int_gpio_num = PIN_TP_INT,
        .levels = {
            .reset = 0,        /* 复位低电平有效 */
            .interrupt = 0,    /* 中断低电平有效 */
        },
        .flags = {
            /* 实测这块屏的触摸 Y 方向是反的（手指点屏幕上边 -> 程序收到下边的坐标），
             * 所以 mirror_y = 1，组件会自动做 y = y_max - y 的软件镜像。
             * 如果以后发现左右也反了，再把 mirror_x 也改成 1 */
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 1,
        },
    };
    ESP_RETURN_ON_ERROR(esp_lcd_touch_new_i2c_cst816s(tp_io, &tp_cfg, &s_touch), TAG, "cst816s");

    /* 4. 把触摸注册为 LVGL 的输入设备（自己注册读回调：带 I2C 容错，绝不崩溃） */
    s_indev = lv_indev_create();
    if (s_indev == NULL) {
        return ESP_FAIL;
    }
    lv_indev_set_type(s_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(s_indev, touch_read_cb);
    lv_indev_set_display(s_indev, display_get());

    /* 事件回调：按下时打印坐标（调试用） */
    if (display_lock(1000)) {
        lv_indev_add_event_cb(s_indev, touch_event_cb, LV_EVENT_PRESSED, NULL);
        display_unlock();
    }

    ESP_LOGI(TAG, "触摸初始化完成（CST816T: SDA=%d SCL=%d RST=%d INT=%d）",
             PIN_TP_SDA, PIN_TP_SCL, PIN_TP_RST, PIN_TP_INT);
    return ESP_OK;
}
