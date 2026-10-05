/*
 * ble_watch.c —— BLE 服务实现（NimBLE）：数据读取 + 蓝牙 OTA
 *
 * 结构（NimBLE 的标准套路）：
 *   1) nimble_port_init() 起协议栈
 *   2) 注册 GAP/GATT 服务 + 我们自己的 0xFF00 服务
 *   3) 同步完成后开始广播；连上以后处理各种事件
 *   4) 收固件：复用 esp_ota_begin/write/end（和 WiFi OTA 完全同一套）
 */
#include "ble_watch.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "watch_version.h"
#include "power_mgr.h"
#include "pages.h"

static const char *TAG = "ble";

#define DEVICE_NAME  "ESP32-Watch"

/* 自定义 UUID（手机端照抄这几个） */
#define UUID_SVC_WATCH    0xFF00
#define UUID_CHR_VERSION  0xFF01
#define UUID_CHR_STATUS   0xFF02
#define UUID_CHR_CMD      0xFF03
#define UUID_CHR_OTA      0xFF04

static uint8_t  s_own_addr_type;
static uint16_t s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t s_status_val_handle;      /* 0xFF02 的值句柄（通知用） */
static bool     s_notify_enabled;         /* 手机有没有订阅通知 */

/* ---- 蓝牙 OTA 状态 ---- */
static esp_ota_handle_t s_ota_handle;
static const esp_partition_t *s_ota_part;
static uint32_t s_ota_total;
static uint32_t s_ota_received;
static uint16_t s_ota_next_seq;            /* 期望的下一个数据包序号（从 0 开始） */
static bool     s_ota_gap_flag;            /* 当前这个"丢包洞"是否已经报给手机 */
static const char *s_ota_result;           /* "ok"/"fail"：校验结果（手机读状态可见） */
static int      s_ota_last_notify_pct = -100;
static bool     s_ota_active;
static esp_timer_handle_t s_ota_report_timer;
static esp_timer_handle_t s_ota_restart_timer;

/* 调试用计数器：每 2 秒打印一次现场情况（看数据流到底卡在哪一步） */
static uint32_t s_ota_dbg_writes;      /* 收到的数据写请求总数 */
static uint32_t s_ota_dbg_reads;       /* 0xFF02 被读取次数 */
static uint32_t s_ota_dbg_barriers;    /* 屏障命令次数 */
static int      s_ota_dbg_tick;

/* ------------------------------------------------------------------------- */
/*  数据内容                                                                  */
/* ------------------------------------------------------------------------- */
static void build_status(char *buf, size_t len)
{
    int n = snprintf(buf, len, "fw=%s;up=%lus;step=0;bat=--",
                     WATCH_FW_VERSION, (unsigned long)(esp_timer_get_time() / 1000000));

    /* 升级进度/校验结果也放进状态里：手机"读状态"就能拿到（不依赖通知通道）
     * —— 有些手机（比如这台 iPhone）蓝牙通知收不到，但"读"一直是通的 ✓ */
    if (s_ota_active && n > 0 && (size_t)n < len) {
        n += snprintf(buf + n, len - n, ";otaq=%u", (unsigned)s_ota_next_seq);
    }
    if (s_ota_result && n > 0 && (size_t)n < len) {
        snprintf(buf + n, len - n, ";otas=%s", s_ota_result);
    }
}

static void notify_text(const char *text)
{
    if (s_conn_handle == BLE_HS_CONN_HANDLE_NONE || s_status_val_handle == 0) {
        return;
    }
    struct os_mbuf *om = ble_hs_mbuf_from_flat(text, strlen(text));
    if (om == NULL) {
        return;
    }
    ble_gatts_notify_custom(s_conn_handle, s_status_val_handle, om);
}

/* ------------------------------------------------------------------------- */
/*  流控回报：每 100ms 告诉手机"我期待的下一个包号"                            */
/*  手机拿它做窗口流控（别发太快）＋断点补发（丢包自愈）                        */
/* ------------------------------------------------------------------------- */
static void ota_report_cb(void *arg)
{
    (void)arg;
    if (!s_ota_active) {
        return;
    }
    char msg[24];
    snprintf(msg, sizeof(msg), "otaq=%u", (unsigned)s_ota_next_seq);
    notify_text(msg);

    if (++s_ota_dbg_tick % 20 == 0) {              /* 每 2 秒打印一次现场情况 */
        ESP_LOGI(TAG, "OTA现场: 数据写%lu次 已收%lu/%lu字节(第%u包) 手机读%lu次 屏障%lu次",
                 (unsigned long)s_ota_dbg_writes, (unsigned long)s_ota_received,
                 (unsigned long)s_ota_total, (unsigned)s_ota_next_seq,
                 (unsigned long)s_ota_dbg_reads, (unsigned long)s_ota_dbg_barriers);
    }
}

static void ota_report_start(void)
{
    if (s_ota_report_timer == NULL) {
        esp_timer_create_args_t args = {
            .callback = ota_report_cb,
            .name = "ota_report",
        };
        if (esp_timer_create(&args, &s_ota_report_timer) != ESP_OK) {
            s_ota_report_timer = NULL;
            return;
        }
    }
    esp_timer_stop(s_ota_report_timer);                       /* 可能在跑，先停 */
    esp_timer_start_periodic(s_ota_report_timer, 100 * 1000); /* 100ms */
}

static void ota_report_stop(void)
{
    if (s_ota_report_timer) {
        esp_timer_stop(s_ota_report_timer);
    }
}

/* 升级成功后延迟重启（用定时器而不是 vTaskDelay：让蓝牙任务空出来，
 * 这 1.5 秒里手机还能"读状态"看到 otas=ok） */
static void ota_restart_cb(void *arg)
{
    (void)arg;
    esp_restart();
}

/* ------------------------------------------------------------------------- */
/*  特征值读写回调                                                             */
/* ------------------------------------------------------------------------- */

/* 0xFF01：版本（读） */
static int chr_version_access(uint16_t conn, uint16_t attr,
                              struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn; (void)attr; (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) {
        return BLE_ATT_ERR_UNLIKELY;
    }
    const char *ver = WATCH_FW_VERSION;
    int rc = os_mbuf_append(ctxt->om, ver, strlen(ver));
    return (rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

/* 0xFF02：状态数据（读）；通知的订阅(CCCD)也走这里 */
static int chr_status_access(uint16_t conn, uint16_t attr,
                             struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn; (void)attr; (void)arg;

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        s_ota_dbg_reads++;
        char buf[64];
        build_status(buf, sizeof(buf));
        int rc = os_mbuf_append(ctxt->om, buf, strlen(buf));
        return (rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_DSC) {          /* 读"是否订阅" */
        uint16_t val = s_notify_enabled ? 1 : 0;
        int rc = os_mbuf_append(ctxt->om, &val, sizeof(val));
        return (rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_DSC) {         /* 手机开/关通知 */
        uint16_t val = 0;
        ble_hs_mbuf_to_flat(ctxt->om, &val, sizeof(val), NULL);
        s_notify_enabled = (val == 0x0001);
        ESP_LOGI(TAG, "手机%s了通知", s_notify_enabled ? "打开" : "关闭");
        return 0;
    }

    return BLE_ATT_ERR_UNLIKELY;
}

/* 0xFF03：命令（写）：目前支持 "reboot" 测试用 */
static int chr_cmd_access(uint16_t conn, uint16_t attr,
                          struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn; (void)attr; (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    char buf[64] = {0};
    uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
    if (len >= sizeof(buf)) {
        len = sizeof(buf) - 1;
    }
    ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &len);

    /* ---- 升级专用命令（都走"带响应写"，可靠同步；藏在命令特征值里）----
     *   "q"       查询：升级开始了吗（手机用它确认 4 字节头送到了）
     *   "b<next>" 屏障：这批数据发完了，手表应已收到第 next 包；
     *             没到齐就回 ATT 错误，手机收到错误会自动补发 ✓
     *   "r"       校验结果查询：校验通过回 0（失败/还没好回错误）
     *   这就是整个升级协议不依赖"通知"的原因：全靠请求-应答 ✓ */
    if (buf[0] == 'b') {
        unsigned long n = strtoul(buf + 1, NULL, 10);
        s_ota_dbg_barriers++;
        if (!s_ota_active) {
            return BLE_ATT_ERR_UNLIKELY;
        }
        /* ★ 容忍度：屏障常常"抢跑"1~4 个包（最后几包还在手机本地队列里排队，
         *   屏障先到了）——差这点就算过，手机继续往下发，那几个包马上也到 ✓
         *   （不设容忍度的话：每次都判"没到齐"-> 批次被反复砍小 -> 传输奇慢，实测踩过）*/
        if ((uint32_t)s_ota_next_seq + 4 >= n) {
            return 0;                      /* 算到齐 ✓ */
        }
        if ((uint32_t)s_ota_next_seq >= n) {
            return 0;                      /* 完全到齐 ✓ */
        }
        ESP_LOGW(TAG, "屏障未到齐：期望收到第 %lu 包，实际只到第 %u 包（等手机补发）",
                 n, (unsigned)s_ota_next_seq);
        if (!s_ota_gap_flag) {             /* 通知能用的话也提一嘴 */
            s_ota_gap_flag = true;
            char msg[24];
            snprintf(msg, sizeof(msg), "otag=%u", (unsigned)s_ota_next_seq);
            notify_text(msg);
        }
        return BLE_ATT_ERR_UNLIKELY;       /* 明确告诉手机"没到齐" */
    }
    if (buf[0] == 'q') {
        return s_ota_active ? 0 : BLE_ATT_ERR_UNLIKELY;
    }
    if (buf[0] == 'r' && buf[1] == '\0') {
        return (s_ota_result && strcmp(s_ota_result, "ok") == 0)
                   ? 0 : BLE_ATT_ERR_UNLIKELY;
    }

    ESP_LOGI(TAG, "收到命令: %.*s", len, buf);

    if (strncmp(buf, "reboot", 6) == 0) {
        notify_text("rebooting");
        vTaskDelay(pdMS_TO_TICKS(300));
        esp_restart();
    }
    return 0;
}

/* 0xFF04：固件数据（写）
 *   4 字节小端总长度（开始/重开） + 若干 [2字节序号 + 固件数据] 数据块 */
static int chr_ota_access(uint16_t conn, uint16_t attr,
                          struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn; (void)attr; (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
    uint8_t buf[520];
    if (len > sizeof(buf)) {
        ESP_LOGE(TAG, "数据块太大: %u", len);
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &len);

    /* ---- 数据总长度头：4 字节小端 = 固件总大小 ----
     * ★ 它同时是"开始/重新开始"信号：手机重试时会再发一次，这里直接重开
     *   （数据块最短 5 字节，不会和 4 字节的头混淆：手机保证每块 ≥ 5）*/
    if (len == 4) {
        if (s_ota_active) {
            esp_ota_abort(s_ota_handle);
            s_ota_active = false;
            ota_report_stop();
            ESP_LOGW(TAG, "收到新的总长度头：放弃上一轮没传完的升级，重新开始");
        }

        s_ota_total = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) |
                      ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);

        s_ota_part = esp_ota_get_next_update_partition(NULL);
        if (s_ota_part == NULL || s_ota_total == 0 || s_ota_total > s_ota_part->size) {
            ESP_LOGE(TAG, "长度/分区不合法：total=%lu 分区=%s",
                     (unsigned long)s_ota_total, s_ota_part ? s_ota_part->label : "无");
            s_ota_total = 0;
            return BLE_ATT_ERR_UNLIKELY;
        }

        /* ★ 用 OTA_SIZE_UNKNOWN：边传边擦（每包几十毫秒）
         *   如果用已知大小，esp_ota_begin 会先把整个 1.6MB 擦完（15~20 秒），
         *   手机那边的 ATT 回应直接超时 -> 蓝牙传固件卡在 0%（踩过的坑）*/
        esp_err_t err = esp_ota_begin(s_ota_part, OTA_SIZE_UNKNOWN, &s_ota_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_begin 失败: %s", esp_err_to_name(err));
            return BLE_ATT_ERR_UNLIKELY;
        }

        s_ota_received = 0;
        s_ota_next_seq = 0;
        s_ota_gap_flag = false;
        s_ota_last_notify_pct = -100;
        s_ota_result = NULL;
        s_ota_dbg_writes = 0;
        s_ota_dbg_reads = 0;
        s_ota_dbg_barriers = 0;
        s_ota_dbg_tick = 0;
        s_ota_active = true;
        power_mgr_keep_awake(true);     /* 传固件期间别睡觉 */
        ota_report_start();             /* 每 100ms 回报进度（手机的流控依据） */
        pages_ble_ota_show();           /* 手表屏幕：切进度页 + 锁住其它操作 */
        ESP_LOGI(TAG, "OTA 开始：%lu 字节 -> 写进 %s",
                 (unsigned long)s_ota_total, s_ota_part->label);
        notify_text("ota=0%");
        return 0;
    }

    if (!s_ota_active) {
        ESP_LOGW(TAG, "还没收到总长度头就来了数据包（%u 字节）", len);
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    s_ota_dbg_writes++;

    /* ---- 数据块：[2 字节小端序号][固件内容] ---- */
    if (len < 2) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }
    uint16_t seq = (uint16_t)(buf[0] | (buf[1] << 8));

    if (seq == s_ota_next_seq) {
        esp_err_t err = esp_ota_write(s_ota_handle, buf + 2, len - 2);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write 失败: %s", esp_err_to_name(err));
            goto fail;
        }
        s_ota_received += len - 2;
        s_ota_next_seq++;
        s_ota_gap_flag = false;

        int pct = (int)((uint64_t)s_ota_received * 100 / s_ota_total);
        if (pct - s_ota_last_notify_pct >= 1 || pct >= 100) {
            /* 每 1% 通知一次：手机界面显示"手表已写入 x%" */
            s_ota_last_notify_pct = pct;
            pages_ble_ota_progress(pct);    /* 手表屏幕的进度数字 */
            char msg[24];
            snprintf(msg, sizeof(msg), "ota=%d%%", pct);
            notify_text(msg);
            if (pct % 10 == 0) {
                ESP_LOGI(TAG, "OTA 进度 %d%%", pct);
            }
        }
    } else if (seq > s_ota_next_seq) {
        /* 中间的包丢了：立刻请手机从缺口处重发（补上后自动复位） */
        if (!s_ota_gap_flag) {
            s_ota_gap_flag = true;
            char msg[24];
            snprintf(msg, sizeof(msg), "otag=%u", (unsigned)s_ota_next_seq);
            notify_text(msg);
            ESP_LOGW(TAG, "丢包：期望 %u 收到 %u（请手机补发）",
                     (unsigned)s_ota_next_seq, (unsigned)seq);
        }
    }
    /* seq < s_ota_next_seq：重复包，直接忽略 */

    if (s_ota_received >= s_ota_total) {
        /* ---- 收满了：校验 + 切换启动分区 + 重启 ---- */
        esp_err_t err = esp_ota_end(s_ota_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "固件校验失败: %s", esp_err_to_name(err));
            goto fail;
        }
        err = esp_ota_set_boot_partition(s_ota_part);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "切换启动分区失败: %s", esp_err_to_name(err));
            goto fail;
        }
        s_ota_active = false;
        ota_report_stop();
        s_ota_result = "ok";               /* 手机"读状态"能看到（通知收不到也 OK） */
        pages_ble_ota_done();              /* 手表屏幕：显示 100% + 完成 */
        ESP_LOGI(TAG, "蓝牙 OTA 成功！1.5 秒后重启进新固件");
        notify_text("ota=ok");             /* 通知也发一份（能收到的手机更快） */
        if (s_ota_restart_timer == NULL) {
            esp_timer_create_args_t args = {
                .callback = ota_restart_cb,
                .name = "ota_restart",
            };
            esp_timer_create(&args, &s_ota_restart_timer);
        }
        if (s_ota_restart_timer) {
            esp_timer_start_once(s_ota_restart_timer, 1500 * 1000);   /* 1.5 秒后重启 */
        } else {
            vTaskDelay(pdMS_TO_TICKS(1500));
            esp_restart();
        }
        return 0;
    }
    return 0;

fail:
    if (s_ota_active) {
        esp_ota_abort(s_ota_handle);
        s_ota_active = false;
    }
    ota_report_stop();
    s_ota_result = "fail";                 /* 手机"读状态"能看到 */
    pages_ble_ota_fail();                  /* 手表屏幕：显示失败，几秒后自动返回 */
    power_mgr_keep_awake(false);
    notify_text("ota=fail");
    return BLE_ATT_ERR_UNLIKELY;
}

/* ------------------------------------------------------------------------- */
/*  GATT 服务表                                                               */
/* ------------------------------------------------------------------------- */
/* ★ 通知的订阅描述符(CCCD)千万不要手写！
 *   NimBLE 看到 NOTIFY 标志会自动加一个，手写会变成"同一个特征值两个 0x2902"，
 *   iOS 会挑错那个 -> 通知死活收不到（踩过的坑） */
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(UUID_SVC_WATCH),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(UUID_CHR_VERSION),
                .access_cb = chr_version_access,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                .uuid = BLE_UUID16_DECLARE(UUID_CHR_STATUS),
                .access_cb = chr_status_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &s_status_val_handle,
            },
            {
                .uuid = BLE_UUID16_DECLARE(UUID_CHR_CMD),
                .access_cb = chr_cmd_access,
                .flags = BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid = BLE_UUID16_DECLARE(UUID_CHR_OTA),
                .access_cb = chr_ota_access,
                /* ★ 只留"无响应写"：小程序才会用"写命令"快速连发（速度快的前提）
                 *   可靠性不靠 ATT 回应，而是靠数据块里的序号：
                 *   手表每 100ms 通知 "otaq=下一个期望的包号"，手机据此流控+补发 ✓
                 *   （带响应写每包要等一个往返 ≈220ms，1.6MB 要 30 分钟，太慢 ✗）*/
                .flags = BLE_GATT_CHR_F_WRITE_NO_RSP,
            },
            { 0 }
        },
    },
    { 0 }
};

/* ------------------------------------------------------------------------- */
/*  广播 / 连接事件                                                            */
/* ------------------------------------------------------------------------- */
static int gap_event_handler(struct ble_gap_event *event, void *arg);

/* 连上以后把链路调快（注意：连接参数和 PHY 两个过程必须"一前一后"发，
 * 同时发会互相冲突、被手机拒绝 —— 日志里那个"553/554 被拒"就是这个原因） */
static void request_conn_params(uint16_t conn)
{
    struct ble_gap_upd_params params = {
        .itvl_min = 12,               /* 15ms */
        .itvl_max = 24,               /* 30ms */
        .latency = 0,                 /* 不偷懒：每个连接事件都能收发 */
        .supervision_timeout = 500,   /* 5 秒（iOS 比较认这个值） */
        .min_ce_len = 0,
        .max_ce_len = 0,
    };
    int rc = ble_gap_update_params(conn, &params);
    ESP_LOGI(TAG, "请求连接参数(15~30ms): rc=%d", rc);
}

static void start_advertising(void)
{
    struct ble_hs_adv_fields fields = {0};
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)DEVICE_NAME;
    fields.name_len = strlen(DEVICE_NAME);
    fields.name_is_complete = 1;

    static ble_uuid16_t svc_uuid = BLE_UUID16_INIT(UUID_SVC_WATCH);
    fields.uuids16 = &svc_uuid;
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "广播数据设置失败: %d", rc);
        return;
    }

    struct ble_gap_adv_params adv = {0};
    adv.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(s_own_addr_type, NULL, BLE_HS_FOREVER, &adv,
                           gap_event_handler, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "广播启动失败: %d", rc);
    }
}

static int gap_event_handler(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            s_conn_handle = event->connect.conn_handle;
            power_mgr_keep_awake(true);
            power_mgr_request_wake_screen();   /* 手机连上 -> 屏幕黑着就点亮 */
            ESP_LOGI(TAG, "手机已连接（连接期间不让手表睡着）");
            if (s_ota_active) {              /* 清掉上次没传完的残留（手机 App 重启过） */
                esp_ota_abort(s_ota_handle);
                s_ota_active = false;
                ota_report_stop();
                pages_ble_ota_fail();
            }
            request_conn_params(s_conn_handle);   /* 先发连接参数（一次只发一个过程） */
        } else {
            start_advertising();
        }
        break;

    case BLE_GAP_EVENT_CONN_UPDATE:
        if (event->conn_update.status == 0) {
            struct ble_gap_conn_desc desc;
            if (ble_gap_conn_find(event->conn_update.conn_handle, &desc) == 0) {
                ESP_LOGI(TAG, "连接参数已更新：间隔 %u (x1.25ms)，延迟 %u",
                         desc.conn_itvl, desc.conn_latency);
            }
        } else {
            ESP_LOGW(TAG, "连接参数更新被拒: %d", event->conn_update.status);
        }
        /* 注意：不要再主动请求 2M PHY！手机会自己升级 PHY（iOS 会主动做），
         * 我们请求反而和手机的 PHY 过程撞车，触发控制器断言（踩过的坑）*/
        break;

    case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE:
        ESP_LOGI(TAG, "PHY 更新完成：tx=0x%02x rx=0x%02x（0x02 = 2M 高速 ✓）",
                 event->phy_updated.tx_phy, event->phy_updated.rx_phy);
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "手机断开（原因 %d），重新广播", event->disconnect.reason);
        s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        s_notify_enabled = false;
        if (s_ota_active) {                  /* 传一半断了：放弃这次升级 */
            esp_ota_abort(s_ota_handle);
            s_ota_active = false;
            ota_report_stop();
            pages_ble_ota_fail();            /* 手表屏幕：显示中断，几秒后返回 */
            ESP_LOGW(TAG, "OTA 中断（不影响现在跑着的固件）");
        }
        power_mgr_keep_awake(false);
        start_advertising();
        break;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "MTU 协商结果: %d（越大传固件越快）", event->mtu.value);
        break;

    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI(TAG, "手机订阅状态: notify=%d", event->subscribe.cur_notify);
        s_notify_enabled = event->subscribe.cur_notify;
        break;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        start_advertising();
        break;

    default:
        break;
    }
    return 0;
}

static void on_sync(void)
{
    ble_hs_util_ensure_addr(0);
    ble_hs_id_infer_auto(0, &s_own_addr_type);
    start_advertising();
    ESP_LOGI(TAG, "BLE 广播中：设备名 \"%s\"，服务 UUID 0x%04X", DEVICE_NAME, UUID_SVC_WATCH);
}

static void ble_host_task(void *param)
{
    (void)param;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/* ------------------------------------------------------------------------- */
/*  启动                                                                      */
/* ------------------------------------------------------------------------- */
esp_err_t ble_watch_start(void)
{
    esp_err_t err = nimble_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_init 失败: %s", esp_err_to_name(err));
        return err;
    }

    ble_svc_gap_device_name_set(DEVICE_NAME);
    ble_svc_gap_init();
    ble_svc_gatt_init();

    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);

    ble_hs_cfg.sync_cb = on_sync;
    nimble_port_freertos_init(ble_host_task);

    ESP_LOGI(TAG, "蓝牙就绪（NimBLE）：手机装 nRF Connect 就能连上看版本/数据");
    return ESP_OK;
}

/*
 * 睡醒后把广播重新拉起来
 * （light sleep 期间射频停了 -> 广播也停了，不重新开的话手机就永远扫不到）
 */
void ble_watch_on_wake(void)
{
    if (s_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        return;                     /* 还连着手机，不用管 */
    }
    if (ble_gap_adv_active()) {
        return;                     /* 广播还在 */
    }
    ESP_LOGI(TAG, "睡醒后重新开始广播");
    start_advertising();
}
