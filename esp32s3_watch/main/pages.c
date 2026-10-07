/*
 * pages.c —— 四个页面：表盘 / 菜单 / 秒表 / 关于
 *
 * 交互设计（触摸为主，BOOT 键为辅）：
 *   返回：  ① 屏幕任意位置向右滑 35 像素
 *           ② 点按子页面底部的 "< BACK" 小字
 *           （BOOT 键长按也能返回）
 *   表盘：  点击 -> 进菜单。（根页面是"边界"：怎么滑都不进/不退页面）
 *   菜单：  点击某一项 -> 进入对应页面（BOOT：短按移光标、长按进入）
 *   秒表：  点击 -> 开始/暂停（BOOT：短按开始暂停、长按清零返回）
 *   关于：  右滑 / 点 "< BACK" / 长按 BOOT 返回
 *
 * 状态灯颜色 = 当前页面：表盘蓝(心跳) / 菜单绿 / 秒表黄 / 关于紫
 *
 * 注意（踩过的坑）：
 *  1) LVGL 的事件回调运行在 LVGL 任务里，锁是"递归锁"，回调里可直接调
 *     page_manager_push/pop（内部再次加锁，安全）。
 *  2) 手势判断必须监听"屏幕控件"的 PRESSED/PRESSING 事件：
 *     LVGL 不会把 PRESSING 发给输入设备(indev)本身，只发给被按下的控件。
 *  3) 滑动松手时 LVGL 还会补发一个 CLICKED（点击）事件。如果不处理，
 *     就会出现"右滑返回以后又被点进下一页"的怪现象（比如在菜单上右滑
 *     返回，松手正好落在 Stopwatch 上，结果又进了秒表页）。
 *     所以：滑动一触发就把 s_sw_done 置 1，所有"点击"回调先检查它，
 *     是滑动产生的"假点击"就忽略。
 */
#include "pages.h"
#include "display.h"
#include "wifi_time.h"
#include "ota_update.h"
#include "watch_version.h"
#include "power_mgr.h"
#include "status_led.h"
#include <time.h>
#include "sdkconfig.h"
#include "esp_log.h"

static const char *TAG = "pages";

/* 说明：固件升级统一由手机小程序通过蓝牙完成（见 ble_watch.c 的 0xFF04）；
 * 这里保留的是"WiFi OTA"备用通道（ota_update.c），界面上没有入口了 */
#define OTA_URL  CONFIG_WATCH_OTA_URL

/* ========================================================================== */
/*  蓝牙升级（手机小程序推送）时的界面联动：显示进度 + 锁住所有操作                  */
/*  设计：BLE 任务只改这几个 volatile 变量（绝不动 LVGL），                        */
/*        真正的切页面/刷文字由 LVGL 定时器来做（线程安全 ✓）                       */
/* ========================================================================== */
#define BLE_OTA_UI_NONE  0
#define BLE_OTA_UI_BUSY  1     /* 正在传固件 */
#define BLE_OTA_UI_OK    2     /* 完成，等重启 */
#define BLE_OTA_UI_FAIL  3     /* 失败/中断，几秒后自动返回 */

static volatile int s_ble_ota_ui  = BLE_OTA_UI_NONE;
static volatile int s_ble_ota_pct = 0;
static int s_ble_ota_fail_ms = 0;      /* 失败提示已显示多久（只在 LVGL 任务里改） */

/* 升级期间：所有操作都不响应（触摸/滑动/按键/点按都要查这个） */
bool pages_ui_locked(void)
{
    return s_ble_ota_ui != BLE_OTA_UI_NONE;
}

void pages_ble_ota_show(void)
{
    s_ble_ota_pct = 0;
    s_ble_ota_ui  = BLE_OTA_UI_BUSY;
}

void pages_ble_ota_progress(int pct)
{
    if (pct >= 0 && pct <= 100) {
        s_ble_ota_pct = pct;
    }
}

void pages_ble_ota_done(void)
{
    s_ble_ota_pct = 100;
    s_ble_ota_ui  = BLE_OTA_UI_OK;
}

void pages_ble_ota_fail(void)
{
    s_ble_ota_fail_ms = 0;
    s_ble_ota_ui = BLE_OTA_UI_FAIL;
}

/* 200ms 轮询：切进度页 / 失败后自动返回（跑在 LVGL 任务里，操作界面安全） */
static void ble_ota_ui_timer_cb(lv_timer_t *t)
{
    (void)t;
    int st = s_ble_ota_ui;
    if (st == BLE_OTA_UI_NONE) {
        return;
    }

    /* 升级开始/进行中：把进度页切到最前（升级时用户看到的只有它） */
    if ((st == BLE_OTA_UI_BUSY || st == BLE_OTA_UI_OK) &&
        page_manager_current() != page_ota_get()) {
        page_manager_push(page_ota_get());
    }

    /* 失败/中断：显示 4 秒后自动退回上一页并解锁（不然会卡在进度页上） */
    if (st == BLE_OTA_UI_FAIL) {
        if (page_manager_current() == page_ota_get() && page_manager_depth() > 1) {
            s_ble_ota_fail_ms += 200;
            if (s_ble_ota_fail_ms >= 4000) {
                page_manager_pop();
                s_ble_ota_ui = BLE_OTA_UI_NONE;
            }
        } else {
            s_ble_ota_ui = BLE_OTA_UI_NONE;   /* 进度页没起来（失败来得太早）：直接解锁 */
        }
    }
}

void pages_ble_ota_ui_init(void)
{
    if (!display_lock(1000)) {
        ESP_LOGE(TAG, "蓝牙升级界面: LVGL 加锁失败");
        return;
    }
    lv_timer_create(ble_ota_ui_timer_cb, 200, NULL);
    display_unlock();
}

/* ========================================================================== */
/*  通用：屏幕手势（右滑返回 + "拖动不算点击"）                                    */
/* ========================================================================== */
#define SWIPE_TRIGGER_DX  35    /* 向右滑 35 像素 -> 返回上一页（越小越灵敏） */
#define DRAG_SUPPRESS_PX  18    /* 位移超过 18px 且"真的在滑动" -> 松手不算点击 */

static lv_point_t s_sw_start;   /* 本次触摸的起点 */
static lv_point_t s_sw_last;    /* 上一个采样点（用来发现"连点造成的跳变"） */
static int s_prev_move;         /* 上一次采样的位移（判断"是不是真的在滑动"） */
static bool s_sw_done;          /* 本次触摸算"滑动" -> 松手时忽略点击 */
static bool s_back_done;        /* 本次触摸已经触发过返回 */

static void screen_swipe_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    /* 蓝牙升级中：锁住所有手势（滑动不返回、松手的假点击也忽略） */
    if (pages_ui_locked()) {
        s_sw_done = true;
        s_back_done = true;
        return;
    }

    lv_indev_t *indev = lv_indev_active();
    if (indev == NULL) {
        return;
    }

    lv_point_t p;
    lv_indev_get_point(indev, &p);

    if (code == LV_EVENT_PRESSED) {
        s_sw_start = p;
        s_sw_last = p;
        s_prev_move = 0;
        s_sw_done = false;
        s_back_done = false;

        /* 熄屏睡觉时被触摸唤醒：这一下只当"点亮屏幕"，不执行任何操作 */
        if (power_mgr_is_waking()) {
            s_sw_done = true;      /* 松手时 LVGL 补发的"假点击"也忽略 */
            s_back_done = true;    /* 也不触发滑动返回 */
        }
    } else if (code == LV_EVENT_PRESSING) {
        int dx = p.x - s_sw_start.x;
        int dy = p.y - s_sw_start.y;
        int adx = (dx < 0) ? -dx : dx;
        int ady = (dy < 0) ? -dy : dy;
        int move = (adx > ady) ? adx : ady;

        /* ① 连点跳变：一次采样跳 >28px，说明触摸芯片把"两次点按"连成了一次触摸。
         *    当成新的一次按下，重新开始判断（不然会被误判成滑动，把页面滑走） */
        int jx = p.x - s_sw_last.x;
        int jy = p.y - s_sw_last.y;
        if (jx < 0) jx = -jx;
        if (jy < 0) jy = -jy;
        s_sw_last = p;
        if (jx > 28 || jy > 28) {
            ESP_LOGI(TAG, "连点跳变：重新开始判断");
            s_sw_start = p;
            s_prev_move = 0;
            s_sw_done = false;
            s_back_done = false;
            return;
        }

        /* ② "真的在滑动" = 位移够大 + 上一次采样也在移动 + 这次还在继续变大。
         *    连点那种"跳一下就不动了"不满足，所以既不会误判成滑动，
         *    也不会把点击吞掉（LVGL 松手时补发的假点击要用 s_sw_done 过滤） */
        bool moving = (move > DRAG_SUPPRESS_PX) && (s_prev_move > 10) && (move > s_prev_move);
        if (moving) {
            s_sw_done = true;
        }

        /* ③ 横向滑动够远：右滑返回；左滑不动作 */
        if (!s_back_done && moving && adx > SWIPE_TRIGGER_DX && adx > ady) {
            s_back_done = true;
            s_sw_done = true;

            if (dx <= 0) {
                ESP_LOGI(TAG, "左滑：不动作（只有右滑返回）");
            } else if (page_manager_depth() > 1) {
                ESP_LOGI(TAG, "右滑返回 (dx=%d dy=%d)", dx, dy);
                lv_indev_stop_processing(indev);   /* 本次触摸不再往下处理 */
                page_manager_pop();
            } else {
                /* 表盘是根页面：边界，不能再往回走了 */
                ESP_LOGI(TAG, "根页面右滑：边界，忽略");
            }
        }

        s_prev_move = move;
    }
}

/* 给页面屏幕接上"右滑返回"手势 */
static void add_swipe_back(lv_obj_t *scr)
{
    lv_obj_add_event_cb(scr, screen_swipe_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(scr, screen_swipe_cb, LV_EVENT_PRESSING, NULL);
}

/* ========================================================================== */
/*  通用：子页面底部 "< BACK" 小字（点按返回）                                     */
/* ========================================================================== */
static void back_label_cb(lv_event_t *e)
{
    if (pages_ui_locked()) {
        return;                 /* 升级中：不让走 */
    }
    if (power_mgr_is_waking()) {
        return;                 /* 刚睡醒那一下只当"点亮屏幕" */
    }
    if (lv_event_get_code(e) == LV_EVENT_PRESSED) {
        ESP_LOGI(TAG, "[BACK] 按下");
        return;
    }
    ESP_LOGI(TAG, "点按返回");
    page_manager_pop();
}

/*
 * 给子页面底部加一行 "< BACK" 小字，点它返回。
 * 故意不加 LV_OBJ_FLAG_EVENT_BUBBLE：一旦冒泡，它的 CLICKED 会冒到屏幕，
 * 秒表页会误触发一次"开始/暂停"。
 */
static void add_back_label(lv_obj_t *scr)
{
    /* 用普通容器(不是 label)做按钮：label 直接当按钮点不太灵，容器最可靠 */
    lv_obj_t *btn = lv_obj_create(scr);
    lv_obj_set_size(btn, 200, 40);
    /* 位置：放在屏幕下部中间（y≈200~240），手指够得着 */
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -44);
    /* 做成一眼能看出"可以点"的小按钮（半透明深色圆角） */
    lv_obj_set_style_bg_opa(btn, LV_OPA_40, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x303030), 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x555555), 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_scrollable(btn, false);
    lv_obj_add_event_cb(btn, back_label_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(btn, back_label_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lab = lv_label_create(btn);
    lv_obj_set_style_text_font(lab, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lab, lv_color_hex(0xDDDDDD), 0);
    lv_label_set_text(lab, "< BACK");
    lv_obj_center(lab);
}

/* ========================================================================== */
/*  通用：做一个"按钮"（圆角半透明 + 居中文字），返回对象                          */
/* ========================================================================== */
static lv_obj_t *make_button(lv_obj_t *parent, const char *text, int w, int h,
                             lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_obj_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_bg_opa(btn, LV_OPA_40, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x303030), 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x555555), 0);
    lv_obj_set_style_pad_all(btn, 0, 0);
    lv_obj_set_scrollable(btn, false);
    if (cb != NULL) {
        lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    }

    lv_obj_t *lab = lv_label_create(btn);
    lv_obj_set_style_text_font(lab, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lab, lv_color_hex(0xDDDDDD), 0);
    lv_label_set_text(lab, text);
    lv_obj_center(lab);
    return btn;
}

/* ========================================================================== */
/*  页面 1：表盘                                                                */
/* ========================================================================== */
static lv_obj_t *s_wf_time;
static lv_obj_t *s_wf_date;

static void wf_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    static bool led_on;
    static int last_min = -1;
    static int last_synced = -1;

    /* 实时时间：联网 NTP 对时后就是准的；没联网时 = 固件编译时间兜底 */
    time_t now = wifi_time_now();
    struct tm tm_now;
    localtime_r(&now, &tm_now);

    if (tm_now.tm_min != last_min) {          /* 分钟没变就不刷文字 */
        last_min = tm_now.tm_min;
        static const char *wd[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
        lv_label_set_text_fmt(s_wf_time, "%02d:%02d", tm_now.tm_hour, tm_now.tm_min);
        if (s_wf_date) {
            lv_label_set_text_fmt(s_wf_date, "%02d/%02d %s",
                                  tm_now.tm_mon + 1, tm_now.tm_mday, wd[tm_now.tm_wday % 7]);
        }
    }

    int synced = wifi_time_is_synced() ? 1 : 0;
    if (synced != last_synced) {              /* 对时状态变了才刷一次日志 */
        last_synced = synced;
        ESP_LOGI(TAG, "对时状态: %s", synced ? "已对时(NTP OK)" : "未对时(用编译时间)");
    }

    /* 只有停在表盘页时才做心跳灯 */
    if (page_manager_current() == page_watchface_get()) {
        led_on = !led_on;
        status_led_set(0, 0, led_on ? 24 : 2);
        ESP_LOGI(TAG, "表盘 %02d:%02d:%02d%s", tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec,
                 synced ? "" : "（未对时，用编译时间）");
    }
}

/* 点击表盘 -> 进菜单 */
static void wf_clicked_cb(lv_event_t *e)
{
    (void)e;
    if (pages_ui_locked()) {
        return;                 /* 升级中：不响应 */
    }
    if (s_sw_done) {
        ESP_LOGI(TAG, "忽略假点击（表盘）");
        return;                 /* 滑动松手的"假点击"，忽略 */
    }
    page_manager_push(page_menu_get());
}

static lv_obj_t *wf_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(scr, false);
    lv_obj_add_event_cb(scr, wf_clicked_cb, LV_EVENT_CLICKED, NULL);
    add_swipe_back(scr);

    s_wf_time = lv_label_create(scr);
    lv_obj_set_style_text_color(s_wf_time, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_wf_time, &lv_font_montserrat_48, 0);
    lv_label_set_text(s_wf_time, "--:--");
    lv_obj_align(s_wf_time, LV_ALIGN_CENTER, 0, -40);

    /* 日期（NTP 对时后自动变准） */
    s_wf_date = lv_label_create(scr);
    lv_obj_set_style_text_color(s_wf_date, lv_color_hex(0x00C8FF), 0);
    lv_obj_set_style_text_font(s_wf_date, &lv_font_montserrat_20, 0);
    lv_label_set_text(s_wf_date, "--/-- ---");
    lv_obj_align(s_wf_date, LV_ALIGN_CENTER, 0, 5);

    lv_obj_t *sub = lv_label_create(scr);
    lv_obj_set_style_text_color(sub, lv_color_hex(0x00C8FF), 0);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_20, 0);
    lv_label_set_text(sub, "ESP32-S3 Watch");
    lv_obj_align(sub, LV_ALIGN_CENTER, 0, 42);

    lv_obj_t *hint = lv_label_create(scr);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x666666), 0);
    lv_label_set_text(hint, "TAP -> MENU");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);

    lv_timer_create(wf_timer_cb, 1000, NULL);
    return scr;
}

static void wf_on_key(button_event_t ev)
{
    if (ev == BTN_EVENT_SHORT_PRESS) {
        page_manager_push(page_menu_get());
    }
}

/* ========================================================================== */
/*  页面 2：菜单                                                                */
/* ========================================================================== */
#define MENU_ITEM_COUNT 2
static lv_obj_t *s_menu_items[MENU_ITEM_COUNT];
static int s_menu_cursor;

static void menu_refresh(void)
{
    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        bool sel = (i == s_menu_cursor);
        lv_obj_set_style_bg_color(s_menu_items[i],
                                  sel ? lv_color_hex(0x0064C8) : lv_color_hex(0x1A1A1A), 0);
        lv_obj_set_style_border_color(s_menu_items[i],
                                      sel ? lv_color_white() : lv_color_hex(0x444444), 0);
    }
}

/* 点击菜单项 -> 进入对应页面（user_data 里存的是下标） */
static void menu_item_cb(lv_event_t *e)
{
    if (pages_ui_locked()) {
        return;                 /* 升级中：不响应 */
    }
    if (s_sw_done) {
        ESP_LOGI(TAG, "忽略假点击（菜单项）");
        return;                 /* 滑动松手的"假点击"，忽略 */
    }
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx == 0) {
        page_manager_push(page_stopwatch_get());
    } else {
        page_manager_push(page_about_get());
    }
}

static lv_obj_t *menu_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(scr, false);
    add_swipe_back(scr);

    lv_obj_t *title = lv_label_create(scr);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_label_set_text(title, "MENU");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    const char *names[MENU_ITEM_COUNT] = { "Stopwatch", "About" };
    for (int i = 0; i < MENU_ITEM_COUNT; i++) {
        lv_obj_t *item = lv_obj_create(scr);
        lv_obj_set_size(item, 200, 58);
        /* 两项：行高 58（OTA 已收进"关于"页，菜单不再容易误点） */
        lv_obj_align(item, LV_ALIGN_TOP_MID, 0, 66 + i * 62);
        lv_obj_set_style_radius(item, 10, 0);
        lv_obj_set_style_border_width(item, 2, 0);
        lv_obj_set_scrollable(item, false);
        lv_obj_add_event_cb(item, menu_item_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        /* 让菜单项上的按压事件也冒泡到屏幕，这样从菜单项上起滑也能返回 */
        lv_obj_add_flag(item, LV_OBJ_FLAG_EVENT_BUBBLE);
        /* 为了好点：行高加大、两行间距缩小（比"扩大点击区域"的 API 更稳） */

        lv_obj_t *label = lv_label_create(item);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_label_set_text(label, names[i]);
        lv_obj_center(label);

        s_menu_items[i] = item;
    }

    add_back_label(scr);

    s_menu_cursor = 0;
    menu_refresh();
    return scr;
}

static void menu_on_key(button_event_t ev)
{
    if (ev == BTN_EVENT_SHORT_PRESS) {
        s_menu_cursor = (s_menu_cursor + 1) % MENU_ITEM_COUNT;
        if (display_lock(100)) {
            menu_refresh();
            display_unlock();
        }
        ESP_LOGI(TAG, "菜单光标 -> %d", s_menu_cursor);
    } else {
        ESP_LOGI(TAG, "菜单：BOOT 长按 -> 进入 %s（光标 %d）",
                 s_menu_cursor == 0 ? "秒表" : "关于", s_menu_cursor);
        page_manager_push(s_menu_cursor == 0 ? page_stopwatch_get() : page_about_get());
    }
}

/* ========================================================================== */
/*  页面 3：秒表                                                                */
/* ========================================================================== */
static lv_obj_t *s_sw_time;
static lv_obj_t *s_sw_state;
static bool s_sw_running;
static int s_sw_ms;

static void sw_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    static bool last_running;

    if (s_sw_running) {
        s_sw_ms += 100;
        lv_label_set_text_fmt(s_sw_time, "%02d:%02d.%d",
                              (s_sw_ms / 60000) % 100,
                              (s_sw_ms / 1000) % 60,
                              (s_sw_ms / 100) % 10);
    }

    if (s_sw_running != last_running) {
        last_running = s_sw_running;
        lv_label_set_text(s_sw_state, s_sw_running ? "RUNNING" : "PAUSED");
    }
}

/* 点击屏幕 -> 开始/暂停 */
static void sw_clicked_cb(lv_event_t *e)
{
    (void)e;
    if (pages_ui_locked()) {
        return;                 /* 升级中：不响应 */
    }
    if (s_sw_done) {
        ESP_LOGI(TAG, "忽略假点击（秒表）");
        return;                 /* 滑动松手的"假点击"，忽略（不然会误开/停秒表） */
    }
    s_sw_running = !s_sw_running;
    ESP_LOGI(TAG, "秒表 %s（触摸）", s_sw_running ? "开始" : "暂停");
}

/* 返回（给 BOOT 键长按用）：清零并返回 */
static void sw_back_cb(lv_event_t *e)
{
    (void)e;
    s_sw_running = false;
    s_sw_ms = 0;
    lv_label_set_text(s_sw_time, "00:00.0");
    page_manager_pop();
}

/* 点底部 "RESET" 按钮：清零（停在当前页） */
static void sw_reset_cb(lv_event_t *e)
{
    (void)e;
    if (pages_ui_locked() || power_mgr_is_waking()) {
        return;
    }
    s_sw_running = false;
    s_sw_ms = 0;
    lv_label_set_text(s_sw_time, "00:00.0");
    lv_label_set_text(s_sw_state, "PAUSED");
    ESP_LOGI(TAG, "秒表 清零（触摸）");
}

static lv_obj_t *sw_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(scr, false);
    lv_obj_add_event_cb(scr, sw_clicked_cb, LV_EVENT_CLICKED, NULL);
    add_swipe_back(scr);

    lv_obj_t *title = lv_label_create(scr);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x888888), 0);
    lv_label_set_text(title, "STOPWATCH");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    s_sw_time = lv_label_create(scr);
    lv_obj_set_style_text_color(s_sw_time, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_sw_time, &lv_font_montserrat_48, 0);
    lv_label_set_text(s_sw_time, "00:00.0");
    lv_obj_align(s_sw_time, LV_ALIGN_CENTER, 0, -20);

    s_sw_state = lv_label_create(scr);
    lv_obj_set_style_text_color(s_sw_state, lv_color_hex(0x00FF80), 0);
    lv_label_set_text(s_sw_state, "PAUSED");
    lv_obj_align(s_sw_state, LV_ALIGN_CENTER, 0, 40);

    /* 底部两个按钮：RESET 清零 / < BACK 返回
     * （点按屏幕其它地方 = 开始/暂停，不受影响） */
    lv_obj_t *rst = make_button(scr, "RESET", 100, 40, sw_reset_cb);
    lv_obj_align(rst, LV_ALIGN_BOTTOM_MID, -55, -44);
    lv_obj_t *bak = make_button(scr, "< BACK", 100, 40, back_label_cb);
    lv_obj_align(bak, LV_ALIGN_BOTTOM_MID, 55, -44);

    lv_obj_t *hint = lv_label_create(scr);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x666666), 0);
    lv_label_set_text(hint, "TAP: start/pause");
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 46);

    lv_timer_create(sw_timer_cb, 100, NULL);
    return scr;
}

static void sw_on_key(button_event_t ev)
{
    if (ev == BTN_EVENT_SHORT_PRESS) {
        s_sw_running = !s_sw_running;
        ESP_LOGI(TAG, "秒表 %s", s_sw_running ? "开始" : "暂停");
    } else {
        sw_back_cb(NULL);
    }
}

/* ========================================================================== */
/*  页面 4：关于                                                                */
/* ========================================================================== */
static lv_obj_t *about_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(scr, false);
    add_swipe_back(scr);

    lv_obj_t *title = lv_label_create(scr);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_label_set_text(title, "ABOUT");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    lv_obj_t *info = lv_label_create(scr);
    lv_obj_set_style_text_color(info, lv_color_hex(0x00C8FF), 0);
    lv_obj_set_style_text_align(info, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text_fmt(info,
                          "ESP32-S3 Watch\n"
                          "FW v%s\n"
                          "LVGL v9 + ESP-IDF\n"
                          "Screen + Touch OK",
                          WATCH_FW_VERSION);
    /* 说明：固件升级统一走手机小程序（蓝牙 OTA），手表上不再放升级入口 */
    lv_obj_align(info, LV_ALIGN_CENTER, 0, 0);

    add_back_label(scr);

    return scr;
}

static void about_on_key(button_event_t ev)
{
    if (ev == BTN_EVENT_LONG_PRESS) {
        page_manager_pop();
    }
}

/* ========================================================================== */
/*  页面 5：OTA 在线升级                                                        */
/* ========================================================================== */
static lv_obj_t *s_ota_state;
static lv_obj_t *s_ota_pct;

static void ota_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (s_ota_state == NULL) {
        return;
    }

    /* ★ 蓝牙升级（手机小程序推送）优先显示 */
    if (s_ble_ota_ui != BLE_OTA_UI_NONE) {
        lv_label_set_text_fmt(s_ota_pct, "%d%%", s_ble_ota_pct);
        if (s_ble_ota_ui == BLE_OTA_UI_BUSY) {
            lv_label_set_text(s_ota_state, "蓝牙升级中\n请勿操作手表");
            lv_obj_set_style_text_color(s_ota_state, lv_color_hex(0xCCCCCC), 0);
        } else if (s_ble_ota_ui == BLE_OTA_UI_OK) {
            lv_label_set_text(s_ota_state, "升级完成\n正在重启...");
            lv_obj_set_style_text_color(s_ota_state, lv_color_hex(0x40FF40), 0);
        } else {
            lv_label_set_text(s_ota_state, "升级失败/已中断\n（马上自动返回）");
            lv_obj_set_style_text_color(s_ota_state, lv_color_hex(0xFF5050), 0);
        }
        return;
    }

    /* —— 下面是 WiFi OTA 备用通道的显示（界面上没有入口了）—— */
    ota_state_t st = ota_update_state();
    lv_label_set_text(s_ota_state, ota_update_message());

    if (st == OTA_STATE_DOWNLOADING) {
        lv_label_set_text_fmt(s_ota_pct, "%d%%", ota_update_percent());
    } else if (st == OTA_STATE_DONE) {
        lv_label_set_text(s_ota_pct, "100%");
    }

    lv_obj_set_style_text_color(s_ota_state,
                                (st == OTA_STATE_FAILED) ? lv_color_hex(0xFF5050) :
                                (st == OTA_STATE_DONE)   ? lv_color_hex(0x40FF40) :
                                                           lv_color_hex(0xCCCCCC), 0);
}

static lv_obj_t *ota_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_scrollable(scr, false);
    /* ★ 这里故意不加"右滑返回"和 "< BACK"：
     *   升级中就是要锁死、不允许任何方式离开（失败时会自动返回）✓ */

    lv_obj_t *title = lv_label_create(scr);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x888888), 0);
    lv_label_set_text(title, "OTA UPDATE");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *ver = lv_label_create(scr);
    lv_obj_set_style_text_font(ver, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(ver, lv_color_hex(0x666666), 0);
    lv_label_set_text_fmt(ver, "ver %s", WATCH_FW_VERSION);
    lv_obj_align(ver, LV_ALIGN_TOP_MID, 0, 48);

    s_ota_pct = lv_label_create(scr);
    lv_obj_set_style_text_color(s_ota_pct, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_ota_pct, &lv_font_montserrat_48, 0);
    lv_label_set_text(s_ota_pct, "0%");
    lv_obj_align(s_ota_pct, LV_ALIGN_CENTER, 0, -15);

    s_ota_state = lv_label_create(scr);
    lv_obj_set_style_text_font(s_ota_state, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_ota_state, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_align(s_ota_state, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(s_ota_state, "准备中...");
    lv_obj_align(s_ota_state, LV_ALIGN_CENTER, 0, 45);

    lv_timer_create(ota_timer_cb, 300, NULL);
    return scr;
}

static void ota_on_key(button_event_t ev)
{
    if (ev == BTN_EVENT_LONG_PRESS) {
        page_manager_pop();
    }
}

/* ========================================================================== */
/*  页面对象定义                                                                */
/* ========================================================================== */
static page_t s_page_watchface = {
    .name = "watchface", .create = wf_create, .on_key = wf_on_key,
    .led_r = 0, .led_g = 0, .led_b = 24,
};

static page_t s_page_menu = {
    .name = "menu", .create = menu_create, .on_key = menu_on_key,
    .led_r = 0, .led_g = 24, .led_b = 0,
};

static page_t s_page_stopwatch = {
    .name = "stopwatch", .create = sw_create, .on_key = sw_on_key,
    .led_r = 24, .led_g = 16, .led_b = 0,
};

static page_t s_page_about = {
    .name = "about", .create = about_create, .on_key = about_on_key,
    .led_r = 16, .led_g = 0, .led_b = 24,
};

static page_t s_page_ota = {
    .name = "ota", .create = ota_create, .on_key = ota_on_key,
    .led_r = 0, .led_g = 20, .led_b = 20,
};

page_t *page_watchface_get(void) { return &s_page_watchface; }
page_t *page_menu_get(void)      { return &s_page_menu; }
page_t *page_stopwatch_get(void) { return &s_page_stopwatch; }
page_t *page_about_get(void)     { return &s_page_about; }
page_t *page_ota_get(void)       { return &s_page_ota; }
