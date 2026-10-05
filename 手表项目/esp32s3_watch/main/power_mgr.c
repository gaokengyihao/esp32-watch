/*
 * power_mgr.c —— 低功耗实现：空闲熄屏 + light sleep + 唤醒
 *
 * 说明（面试可以讲的点）：
 *   - 为什么用"手动 light sleep"而不是 "自动 light sleep(esp_pm)"？
 *     手动更可控：我们自己决定什么时候睡、睡之前把屏幕/状态灯关掉；
 *     自动的话任何任务一空闲就睡，触摸轮询任务会把芯片一直拽醒，反而更费电。
 *   - 为什么用"唤醒保护期"？睡了以后手指按屏幕是为了唤醒，这一下不应该
 *     再触发"点进菜单"之类的操作，所以醒来 0.6 秒内的事件只用来亮屏。
 *   - 为什么要"5 秒睡 / 5 秒广播"循环？light sleep 期间射频是停的，
 *     但用户不想每次连接都先点屏幕唤醒。于是睡着后每 5 秒醒来开 5 秒
 *     蓝牙广播窗口：手机随时能搜到/连上（不用碰手表），其余时间照常省电。
 *     连上后 keep_awake(true) + 亮屏，进入正常使用。
 */
#include "power_mgr.h"

#include <stdlib.h>

#include "sdkconfig.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "display.h"
#include "status_led.h"
#include "page_manager.h"
#include "ble_watch.h"

static const char *TAG = "power";

#define PIN_TP_INT   7
#define PIN_BUTTON   0

#define IDLE_TO_SLEEP_MS     15000    /* 15 秒没操作：熄屏 + 睡觉 */
#define WAKE_GUARD_MS          600    /* 醒来后 0.6 秒内的操作只当"点亮屏幕" */

/* ★ 睡眠-广播 循环：不用点屏幕也能被手机搜到/连上
 *   睡 5 秒 -> 醒 5 秒（只开蓝牙广播、屏幕不亮）-> 再睡…
 *   手机在这 5 秒窗口里能发现手表；连上就 keep_awake + 亮屏 */
#define SLEEP_CHUNK_US       (5 * 1000 * 1000LL)
#define ADV_WINDOW_MS        5000

static volatile int64_t s_last_activity;   /* 上次活动时间（us） */
static volatile bool s_waking;             /* 刚醒来的保护期 */
static volatile bool s_keep_awake;         /* true = 别睡 */
static volatile bool s_asleep;             /* 正在睡 */
static volatile bool s_screen_request;     /* 手机连上：请求亮屏 */
static int s_fail_cnt;                     /* light sleep 连续失败次数 */
static uint32_t s_sleep_cnt;               /* 睡眠次数（日志降噪用） */

static void notify_activity(void)
{
    s_last_activity = esp_timer_get_time();
}

void power_mgr_notify_activity(void)
{
    notify_activity();
}

void power_mgr_keep_awake(bool on)
{
    s_keep_awake = on;
    notify_activity();
}

void power_mgr_request_wake_screen(void)
{
    s_screen_request = true;
    notify_activity();
}

bool power_mgr_is_waking(void)
{
    return s_waking;
}

static const char *wake_cause_str(uint32_t causes)
{
    if (causes & BIT(ESP_SLEEP_WAKEUP_GPIO))  return "GPIO（触摸/按键）";
    if (causes & BIT(ESP_SLEEP_WAKEUP_TIMER)) return "定时器";
    return "其它";
}

/* 亮屏（触摸/按键唤醒、或手机在广播窗口里连上时用） */
static void wake_screen_now(void)
{
    /* 先给 LCD 重发一遍初始化（万一睡眠期间面板被扰动/复位了，这一步能救回来；
     * 复位后的 ST7789 会回到 sleep-in 状态，不重发初始化就是一片白） */
    display_reinit();
    display_set_on(true);

    /* 黑屏这段时间面板可能丢了画面：标记整屏失效，让 LVGL 任务自己重画。
     * ★ 千万别在这里调 lv_refr_now() 强制渲染！LVGL 整屏渲染要很大的栈，
     *   power 任务的栈不够 -> 会栈溢出重启（踩过的坑）。*/
    if (display_lock(1000)) {
        lv_obj_invalidate(lv_screen_active());
        display_unlock();
    }
    const page_t *p = page_manager_current();
    if (p != NULL) {
        status_led_set(p->led_r, p->led_g, p->led_b);
    }
}

/* 熄屏 + 睡觉；被唤醒后返回 */
static void enter_sleep(void)
{
    /* 手指还按着 / 按键还按着的时候别睡，不然睡了立刻又醒 */
    if (gpio_get_level(PIN_TP_INT) == 0 || gpio_get_level(PIN_BUTTON) == 0) {
        notify_activity();
        return;
    }

    /* 日志降噪：前两次 + 每 5 分钟打印一次完整说明（之前每 5 秒刷一次，日志看不过来） */
    bool loud = (s_sleep_cnt < 2) || (s_sleep_cnt % 60 == 0);
    s_sleep_cnt++;
    if (loud) {
        ESP_LOGI(TAG, "空闲 %d 秒：熄屏 + 进入 light sleep（每 %d 秒开 %d 秒蓝牙广播窗口，手机免唤醒可连）",
                 IDLE_TO_SLEEP_MS / 1000, (int)(SLEEP_CHUNK_US / 1000000), ADV_WINDOW_MS / 1000);
        ESP_LOGI(TAG, "（开发板上背光是直连 3V3 的，屏幕会变成暗灰；电池供电时要改成 MOS 管控制）");
    }

    s_asleep = true;

    /* 先拿住 LVGL 锁：保证此刻没有"刷屏/读触摸"在进行中，
     * 否则睡下去会把正在跑的那次 I2C/SPI 事务挂死（醒来就点不动了） */
    bool locked = display_lock(1000);

    display_set_on(false);          /* 屏幕黑掉（只关显示输出，显存保留） */
    status_led_set(0, 0, 0);        /* 状态灯也关掉省电 */

    vTaskDelay(pdMS_TO_TICKS(200)); /* 等日志发完（睡下去串口就没输出了） */

    esp_err_t err = esp_light_sleep_start();   /* 睡到有唤醒源为止 */
    uint32_t causes = esp_sleep_get_wakeup_causes();
    s_asleep = false;

    if (locked) {
        display_unlock();           /* 亮屏统一走 wake_screen_now()，它会自己再加锁 */
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "light sleep 失败: %s", esp_err_to_name(err));
        if (++s_fail_cnt >= 3) {
            ESP_LOGE(TAG, "light sleep 一直失败，之后只做熄屏不再睡了（检查 PSRAM/串口配置）");
            s_keep_awake = true;
        }
    } else {
        s_fail_cnt = 0;
    }

    /* ============ 情形 A：定时醒来 —— 只开"广播窗口"，屏幕保持黑 ============ */
    if ((causes & BIT(ESP_SLEEP_WAKEUP_GPIO)) == 0) {
        /* 睡一觉射频停了：把广播拉起来，手机就靠这 5 秒窗口找到手表 */
        ble_watch_on_wake();

        /* 在窗口里等手机连上来（连上会 keep_awake(true) + 请求亮屏） */
        for (int i = 0; i < ADV_WINDOW_MS / 100; i++) {
            if (s_keep_awake || s_screen_request) {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        if (s_keep_awake || s_screen_request) {
            s_screen_request = false;
            wake_screen_now();
            ESP_LOGI(TAG, "广播窗口内手机连上了：亮屏 + 连接期间不休眠");
        }
        /* ★ 这里故意不 notify_activity()：主循环马上会进入下一轮睡眠，
         *   形成"5 秒睡 / 5 秒广播"的循环 —— 睡着也能被手机连 ✓ */
        return;
    }

    /* ============ 情形 B：触摸/按键唤醒 —— 正常亮屏 ============ */
    wake_screen_now();
    notify_activity();              /* 这是真·用户活动：重置空闲计时 */

    /* 睡醒的第一下只当"点亮屏幕" */
    s_waking = true;
    vTaskDelay(pdMS_TO_TICKS(WAKE_GUARD_MS));
    s_waking = false;

    ESP_LOGI(TAG, "已唤醒（原因: %s）", wake_cause_str(causes));
}

static void power_task(void *arg)
{
    (void)arg;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        if (s_asleep || s_keep_awake) {
            continue;
        }
        int64_t idle_ms = (esp_timer_get_time() - s_last_activity) / 1000;
        if (idle_ms >= IDLE_TO_SLEEP_MS) {
            enter_sleep();
        }
    }
}

esp_err_t power_mgr_init(void)
{
    notify_activity();

    /* ★★ 关键：告诉芯片"睡下去以后这些引脚别动"！
     * light sleep 默认会把所有 GPIO 隔离（悬空），一旦下面这些引脚悬空：
     *   - LCD RST(14) 被扰动 -> ST7789 复位 -> 回到 sleep-in -> 醒来白屏
     *   - 触摸 RST(6) 被扰动 -> CST816T 复位 -> INT 配置丢 -> 触摸彻底不响应
     * 所以让它们保持"正常配置"（电平不变） */
    static const int keep_same[] = {
        6,      /* 触摸 RST */
        8, 9,   /* 触摸 I2C SDA/SCL */
        10, 11, 12, 13, 14,   /* LCD CS/MOSI/SCLK/DC/RST */
    };
    for (int i = 0; i < (int)(sizeof(keep_same) / sizeof(keep_same[0])); i++) {
        gpio_sleep_sel_dis((gpio_num_t)keep_same[i]);
    }

    /* 唤醒引脚：保持"输入"状态，并确保有上拉（否则悬空会误唤醒/唤不醒） */
    gpio_set_pull_mode(PIN_TP_INT, GPIO_PULLUP_ONLY);
    gpio_sleep_sel_dis(PIN_TP_INT);
    gpio_sleep_sel_dis(PIN_BUTTON);

    /* 唤醒源 1：触摸 INT（低电平有效）
     * ★★ 千万别在这里 gpio_config() 重配 GPIO7！
     *    触摸驱动是用"INT 中断"来触发读坐标的（lv_indev 事件模式），
     *    重新配置 GPIO 会把它的中断关掉 -> 表现是"睡了之后再也点不动"。
     *    （这是踩过的坑，详见 项目说明.md） */
    ESP_RETURN_ON_ERROR(gpio_wakeup_enable(PIN_TP_INT, GPIO_INTR_LOW_LEVEL), TAG, "tp wake");

    /* 唤醒源 2：BOOT 键（低电平有效；GPIO 已由 button.c 配好，这里也只开唤醒） */
    ESP_RETURN_ON_ERROR(gpio_wakeup_enable(PIN_BUTTON, GPIO_INTR_LOW_LEVEL), TAG, "key wake");

    /* 允许 GPIO 唤醒 light sleep */
    ESP_RETURN_ON_ERROR(esp_sleep_enable_gpio_wakeup(), TAG, "gpio wakeup");

    /* 唤醒源 3：定时（睡 5 秒醒一次开"广播窗口"：手机不点屏幕也能搜到/连上） */
    ESP_RETURN_ON_ERROR(esp_sleep_enable_timer_wakeup(SLEEP_CHUNK_US), TAG, "timer wakeup");

    if (xTaskCreate(power_task, "power", 5120, NULL, 2, NULL) != pdPASS) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "低功耗就绪：%d 秒无操作熄屏+睡觉；睡眠中每 %d 秒开 %d 秒蓝牙广播窗口（手机免唤醒可连）",
             IDLE_TO_SLEEP_MS / 1000, (int)(SLEEP_CHUNK_US / 1000000), ADV_WINDOW_MS / 1000);
    return ESP_OK;
}
