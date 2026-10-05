/*
 * ota_update.c —— 在线升级（OTA）实现
 *
 * 面试考点：OTA 是怎么做到"不断电不插线"换固件的？
 *   1) flash 里有两块放程序的槽：ota_0 / ota_1（分区表里配的）
 *   2) 当前跑在 ota_0，就把新固件下载写进 ota_1（绝不能写自己正在跑的那块）
 *   3) 写完把 otadata 改成"下次从 ota_1 启动" -> esp_restart()
 *   4) 新固件跑起来正常 -> esp_ota_mark_app_valid_cancel_rollback() 确认
 *      要是新固件起不来（或忘了确认）-> bootloader 会自动回滚到旧版本，变不了砖
 *
 * 开发时的服务器（电脑上）：
 *   cd 放固件的目录
 *   python -m http.server 8000
 * 手表就从这个地址下载（地址配在 menuconfig -> Watch 手表配置 -> OTA 固件下载地址）
 */
#include "ota_update.h"

#include <string.h>
#include <stdio.h>

#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "wifi_time.h"
#include "power_mgr.h"
#include "watch_version.h"

static const char *TAG = "ota";

#define OTA_URL  CONFIG_WATCH_OTA_URL

static volatile ota_state_t s_state = OTA_STATE_IDLE;
static volatile int s_percent;
static char s_msg[48] = "";
static char s_latest[16] = "";    /* 服务器上的版本号 */

static void set_state(ota_state_t st, const char *msg)
{
    s_state = st;
    if (msg != NULL) {
        strlcpy(s_msg, msg, sizeof(s_msg));
        ESP_LOGI(TAG, "%s", msg);
    }
}

static void ota_task(void *arg)
{
    const char *url = (const char *)arg;
    esp_https_ota_handle_t handle = NULL;
    esp_err_t err;

    power_mgr_keep_awake(true);   /* 升级期间禁止熄屏/睡觉（不然会中断下载） */

    /* 1. 打开 WiFi（对时完已经关了，这里按需再开） */
    set_state(OTA_STATE_CONNECTING, "连接 WiFi...");
    if (wifi_time_wifi_on(20000) != ESP_OK) {
        set_state(OTA_STATE_FAILED, "WiFi 连接失败");
        goto done;
    }

    /* 2. 开始下载（esp_https_ota 会把固件写进"另一块" app 分区） */
    ESP_LOGI(TAG, "开始下载固件: %s", url);
    set_state(OTA_STATE_DOWNLOADING, "下载中...");

    esp_http_client_config_t http_cfg = {
        .url = url,
        .timeout_ms = 20000,
        .keep_alive_enable = true,
    };
    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    err = esp_https_ota_begin(&ota_cfg, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "连不上服务器/没有这个文件: %s", esp_err_to_name(err));
        set_state(OTA_STATE_FAILED, "连不上服务器");
        goto done;
    }

    int total = esp_https_ota_get_image_size(handle);
    ESP_LOGI(TAG, "固件大小: %d 字节", total);

    /* 3. 循环下载（每轮写一块 flash），顺便算进度 */
    while (1) {
        err = esp_https_ota_perform(handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;      /* 下载完成或者出错 */
        }
        if (total > 0) {
            int done = (int)esp_https_ota_get_image_len_read(handle);
            int pct = (int)((int64_t)done * 100 / total);
            s_percent = (pct > 99) ? 99 : pct;
        }
    }

    if (err != ESP_OK || !esp_https_ota_is_complete_data_received(handle)) {
        ESP_LOGE(TAG, "下载失败: %s", esp_err_to_name(err));
        set_state(OTA_STATE_FAILED, "下载失败");
        esp_https_ota_abort(handle);
        goto done;
    }

    /* 4. 校验 + 写 otadata（下次从新分区启动） */
    err = esp_https_ota_finish(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "固件校验失败: %s", esp_err_to_name(err));
        set_state(OTA_STATE_FAILED, "固件校验失败");
        goto done;
    }

    s_percent = 100;
    set_state(OTA_STATE_DONE, "升级完成，正在重启...");
    ESP_LOGI(TAG, "OTA 成功！重启后就是新固件（3 秒）");
    vTaskDelay(pdMS_TO_TICKS(3000));
    wifi_time_wifi_off();   /* 顺手关掉 WiFi（不然重启时会打印一次"WiFi 断开重连"的警告） */
    esp_restart();      /* 重启进新固件 */

done:
    power_mgr_keep_awake(false);
    wifi_time_wifi_off();
    vTaskDelete(NULL);
}

esp_err_t ota_update_start(const char *url)
{
    if (s_state == OTA_STATE_CONNECTING || s_state == OTA_STATE_DOWNLOADING) {
        ESP_LOGW(TAG, "已经在升级中，忽略");
        return ESP_ERR_INVALID_STATE;
    }
    if (url == NULL || url[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    s_percent = 0;
    set_state(OTA_STATE_IDLE, "准备中...");

    if (xTaskCreate(ota_task, "ota", 8192, (void *)url, 5, NULL) != pdPASS) {
        set_state(OTA_STATE_FAILED, "任务创建失败");
        return ESP_FAIL;
    }
    return ESP_OK;
}

/* ========================================================================== */
/*  检查版本：去服务器拿 version.txt，和本机版本比                                */
/* ========================================================================== */

/* 把 OTA 固件地址变成"同目录下的 version.txt"
 *   http://ip:8000/esp32s3_watch.bin  ->  http://ip:8000/version.txt */
static void build_version_url(char *out, size_t out_len)
{
    const char *url = OTA_URL;
    const char *slash = strrchr(url, '/');

    if (slash == NULL) {
        strlcpy(out, url, out_len);
        return;
    }
    size_t n = (size_t)(slash - url) + 1;   /* 含最后一个 '/' */
    if (n >= out_len) {
        n = out_len - 1;
    }
    memcpy(out, url, n);
    out[n] = '\0';
    strlcat(out, "version.txt", out_len);
}

static void check_task(void *arg)
{
    (void)arg;
    char url[160] = {0};
    char buf[32] = {0};
    esp_http_client_handle_t client = NULL;
    bool ok = false;

    s_latest[0] = '\0';
    set_state(OTA_STATE_CHECKING, "检查更新中...");

    if (wifi_time_wifi_on(20000) != ESP_OK) {
        set_state(OTA_STATE_CHECK_FAILED, "没连上 WiFi");
        goto done;
    }

    build_version_url(url, sizeof(url));
    ESP_LOGI(TAG, "检查版本: %s", url);

    esp_http_client_config_t cfg = {
        .url = url,
        .timeout_ms = 10000,
    };
    client = esp_http_client_init(&cfg);
    if (client == NULL) {
        set_state(OTA_STATE_CHECK_FAILED, "初始化失败");
        goto done;
    }

    if (esp_http_client_open(client, 0) == ESP_OK) {
        esp_http_client_fetch_headers(client);
        int rd = esp_http_client_read(client, buf, sizeof(buf) - 1);
        if (rd > 0) {
            buf[rd] = '\0';
            ok = (esp_http_client_get_status_code(client) == 200);
        }
        esp_http_client_close(client);
    }
    esp_http_client_cleanup(client);
    client = NULL;

    if (!ok || buf[0] == '\0') {
        ESP_LOGE(TAG, "检查失败（服务器上要有 version.txt）");
        set_state(OTA_STATE_CHECK_FAILED, "检查失败\n(服务器要有 version.txt)");
        goto done;
    }

    /* 去掉结尾换行/空格 */
    for (int i = 0; buf[i] != '\0'; i++) {
        if (buf[i] == '\r' || buf[i] == '\n' || buf[i] == ' ') {
            buf[i] = '\0';
            break;
        }
    }
    strlcpy(s_latest, buf, sizeof(s_latest));

    if (strcmp(s_latest, WATCH_FW_VERSION) == 0) {
        ESP_LOGI(TAG, "已经是最新版本 v%s", s_latest);
        set_state(OTA_STATE_UP_TO_DATE, "已是最新版本");
    } else {
        ESP_LOGI(TAG, "发现新版本: v%s（当前 v%s）", s_latest, WATCH_FW_VERSION);
        set_state(OTA_STATE_UPDATE_FOUND, "发现新版本，确认后可升级");
    }

done:
    wifi_time_wifi_off();
    vTaskDelete(NULL);
}

esp_err_t ota_update_check_start(void)
{
    if (s_state == OTA_STATE_CONNECTING || s_state == OTA_STATE_DOWNLOADING) {
        ESP_LOGW(TAG, "正在升级中，不检查");
        return ESP_ERR_INVALID_STATE;
    }
    if (s_state == OTA_STATE_CHECKING) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xTaskCreate(check_task, "ota_check", 5120, NULL, 4, NULL) != pdPASS) {
        set_state(OTA_STATE_CHECK_FAILED, "任务创建失败");
        return ESP_FAIL;
    }
    return ESP_OK;
}

const char *ota_update_latest_version(void)
{
    return s_latest;
}

ota_state_t ota_update_state(void)
{
    return s_state;
}

int ota_update_percent(void)
{
    return s_percent;
}

const char *ota_update_message(void)
{
    return s_msg;
}
