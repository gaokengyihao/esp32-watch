/*
 * wifi_time.c —— WiFi + 联网对时（SNTP）
 *
 * 面试考点：嵌入式设备怎么知道"现在几点"？
 *   芯片内部只有一个 RTC 计数器，掉电就丢了，必须找个地方"对表"：
 *     1) WiFi + NTP  ← 本项目现在用的（开机几秒就对好，不用加硬件）
 *     2) 手机 App 用蓝牙把时间发过来（后面做 BLE 时可以加）
 *     3) 外挂 RTC 芯片（DS3231 / PCF8563，带纽扣电池，断网也能走）
 *
 * 流程：
 *   ① 先把系统时间设成"固件编译时间"（__DATE__ / __TIME__）兜底
 *   ② 连 WiFi（STA 模式）
 *   ③ SNTP 向 ntp.aliyun.com 请求时间，成功后系统时间就准了
 *   ④ 设时区：中国 = 东八区，POSIX 写法 "CST-8"（注意符号是反的！）
 *   ⑤ 用完就把 WiFi 关掉：省电 + 避免发射电流让屏幕背光闪
 * 整个过程在一个后台任务里跑，界面先启动，开机不卡。
 *
 * 另外对外提供 wifi_time_wifi_on() / wifi_time_wifi_off()，
 * 给 OTA 这种"要联网才做"的功能按需开关 WiFi。
 */
#include "wifi_time.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "sdkconfig.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

static const char *TAG = "wifi_time";

/* WiFi 名 / 密码 / NTP 服务器：idf.py menuconfig -> "Watch 手表配置" */
#define WIFI_SSID   CONFIG_WATCH_WIFI_SSID
#define WIFI_PASS   CONFIG_WATCH_WIFI_PASSWORD
#define NTP_SERVER  CONFIG_WATCH_NTP_SERVER

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAILED_BIT     BIT1
#define WIFI_RETRY_MAX      3

static EventGroupHandle_t s_wifi_events;
static SemaphoreHandle_t s_wifi_mux;   /* 串行化 WiFi 使用：对时/检查更新/OTA 不能同时抢 */
static bool s_wifi_owned;  /* 当前是不是我们拿着 WiFi（成功连上到关闭之间） */
static bool s_synced;      /* NTP 对时成功过 */
static bool s_wifi_done;   /* 是我们主动关的 WiFi（事件里别再自动重连） */
static bool s_inited;      /* NVS / netif / wifi 只初始化一次 */
static int  s_retry;       /* WiFi 重连次数 */

/* ------------------------------------------------------------------------- */
/*  兜底时间：固件编译时间（__DATE__="Sep 29 2026"  __TIME__="12:34:56"）      */
/* ------------------------------------------------------------------------- */
static void set_time_from_build(void)
{
    static const char *mon_names = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char mon[4] = {0};
    int day = 0, year = 0, hh = 0, mm = 0, ss = 0;

    if (sscanf(__DATE__, "%3s %d %d", mon, &day, &year) != 3) {
        return;
    }
    if (sscanf(__TIME__, "%d:%d:%d", &hh, &mm, &ss) != 3) {
        return;
    }
    const char *p = strstr(mon_names, mon);
    if (p == NULL) {
        return;
    }

    struct tm tm_build = {0};
    tm_build.tm_year = year - 1900;
    tm_build.tm_mon  = (int)(p - mon_names) / 3;
    tm_build.tm_mday = day;
    tm_build.tm_hour = hh;
    tm_build.tm_min  = mm;
    tm_build.tm_sec  = ss;

    /* mktime 按本地时区解释（编译这台电脑就是北京时间，TZ 已设成 CST-8） */
    time_t t = mktime(&tm_build);
    struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&tv, NULL);

    ESP_LOGI(TAG, "还没对时，先用固件编译时间：%04d-%02d-%02d %02d:%02d:%02d",
             year, tm_build.tm_mon + 1, day, hh, mm, ss);
}

/* ------------------------------------------------------------------------- */
/*  WiFi 事件                                                                 */
/* ------------------------------------------------------------------------- */
static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_wifi_done) {
            return;                 /* 是我们主动关的，不用重连 */
        }
        if (s_retry < WIFI_RETRY_MAX) {
            s_retry++;
            ESP_LOGW(TAG, "WiFi 断开，第 %d 次重连...", s_retry);
            esp_wifi_connect();
        } else {
            ESP_LOGE(TAG, "WiFi 连不上：检查名字/密码（menuconfig -> Watch 手表配置），"
                          "以及路由器是不是 2.4G 频段");
            xEventGroupSetBits(s_wifi_events, WIFI_FAILED_BIT);
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "WiFi 已连接，IP = " IPSTR, IP2STR(&ev->ip_info.ip));
        s_retry = 0;
        xEventGroupSetBits(s_wifi_events, WIFI_CONNECTED_BIT);
    }
}

/* ------------------------------------------------------------------------- */
/*  WiFi 底层：NVS / netif / 事件循环 / wifi_init 只做一次                       */
/* ------------------------------------------------------------------------- */
static esp_err_t wifi_init_once(void)
{
    if (s_inited) {
        return ESP_OK;
    }

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init 失败: %s", esp_err_to_name(err));
        return err;
    }

    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "netif");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "event loop");
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&init_cfg), TAG, "wifi init");
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                        wifi_event_handler, NULL, NULL), TAG, "wifi event");
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                        wifi_event_handler, NULL, NULL), TAG, "ip event");

    s_wifi_events = xEventGroupCreate();
    if (s_wifi_events == NULL) {
        return ESP_ERR_NO_MEM;
    }
    s_wifi_mux = xSemaphoreCreateMutex();
    if (s_wifi_mux == NULL) {
        return ESP_ERR_NO_MEM;
    }

    s_inited = true;
    return ESP_OK;
}

esp_err_t wifi_time_wifi_on(uint32_t timeout_ms)
{
    if (WIFI_SSID[0] == '\0') {
        ESP_LOGW(TAG, "没配置 WiFi（idf.py menuconfig -> Watch 手表配置）");
        return ESP_ERR_INVALID_STATE;
    }
    ESP_RETURN_ON_ERROR(wifi_init_once(), TAG, "wifi init once");

    /* 串行化：对时 / 检查更新 / OTA 可能同时想用 WiFi，一个用完才轮到下一个
     * （开机头几秒正在 NTP 对时时点"检查更新"，没这个锁就会失败） */
    if (xSemaphoreTake(s_wifi_mux, pdMS_TO_TICKS(timeout_ms + 5000)) != pdTRUE) {
        ESP_LOGW(TAG, "WiFi 正被占用，稍后再试");
        return ESP_ERR_TIMEOUT;
    }

    s_wifi_done = false;
    s_retry = 0;
    xEventGroupClearBits(s_wifi_events, WIFI_CONNECTED_BIT | WIFI_FAILED_BIT);

    wifi_config_t wc = {0};
    strlcpy((char *)wc.sta.ssid, WIFI_SSID, sizeof(wc.sta.ssid));
    strlcpy((char *)wc.sta.password, WIFI_PASS, sizeof(wc.sta.password));
    wc.sta.threshold.authmode = (WIFI_PASS[0] == '\0') ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "set mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wc), TAG, "set config");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "wifi start");

    /* 不省电：省电模式每隔 ~100ms 发一次突发，电流一抖一抖的会带着屏幕背光闪 */
    esp_wifi_set_ps(WIFI_PS_NONE);
    ESP_LOGI(TAG, "正在连接 WiFi: %s ...", WIFI_SSID);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_events,
                                           WIFI_CONNECTED_BIT | WIFI_FAILED_BIT,
                                           pdFALSE, pdFALSE, pdMS_TO_TICKS(timeout_ms));
    if (!(bits & WIFI_CONNECTED_BIT)) {
        xSemaphoreGive(s_wifi_mux);     /* 没连上：让出 WiFi */
        return ESP_FAIL;
    }

    s_wifi_owned = true;                /* 连上了：拿着 WiFi，等 wifi_time_wifi_off() 释放 */
    return ESP_OK;
}

void wifi_time_wifi_off(void)
{
    if (!s_inited) {
        return;
    }
    s_wifi_done = true;
    esp_wifi_stop();
    ESP_LOGI(TAG, "WiFi 已关闭（省电）");

    if (s_wifi_owned) {
        s_wifi_owned = false;
        xSemaphoreGive(s_wifi_mux);     /* 轮到下一个等 WiFi 的 */
    }
}

bool wifi_time_is_connected(void)
{
    return s_inited && (xEventGroupGetBits(s_wifi_events) & WIFI_CONNECTED_BIT);
}

/* ------------------------------------------------------------------------- */
/*  后台任务：开机连 WiFi -> NTP 对时 -> 关 WiFi                                 */
/* ------------------------------------------------------------------------- */
static void wifi_time_task(void *arg)
{
    (void)arg;

    if (wifi_time_wifi_on(20000) != ESP_OK) {
        ESP_LOGW(TAG, "WiFi 没连上，时间用固件编译时间兜底");
        wifi_time_wifi_off();
        vTaskDelete(NULL);
        return;
    }

    esp_sntp_config_t sntp_cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG(NTP_SERVER);
    ESP_ERROR_CHECK(esp_netif_sntp_init(&sntp_cfg));

    if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(15000)) == ESP_OK) {
        s_synced = true;
        time_t now = time(NULL);
        struct tm tm_now;
        localtime_r(&now, &tm_now);
        ESP_LOGI(TAG, "NTP 对时成功：%04d-%02d-%02d %02d:%02d:%02d（东八区）",
                 tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
                 tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
    } else {
        ESP_LOGE(TAG, "NTP 对时超时（继续用编译时间兜底）");
    }

    /* 对时完就把 WiFi 关掉：
     *   1) 省电 —— 手表上 WiFi 常连是最费电的（第4步低功耗时你就知道了）
     *   2) 避免 WiFi 发射的电流波动让屏幕背光"一闪一闪"
     * OTA / 其它联网功能要用时再按需打开 */
    wifi_time_wifi_off();
    vTaskDelete(NULL);
}

esp_err_t wifi_time_start(void)
{
    /* 时区 + 兜底时间：先做好，这样同步前表盘也有个接近的时间 */
    setenv("TZ", "CST-8", 1);   /* 东八区：POSIX 写法 CST-8 = UTC+8 */
    tzset();
    set_time_from_build();

    if (xTaskCreate(wifi_time_task, "wifi_time", 6144, NULL, 5, NULL) != pdPASS) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

bool wifi_time_is_synced(void)
{
    return s_synced;
}

time_t wifi_time_now(void)
{
    return time(NULL);
}
