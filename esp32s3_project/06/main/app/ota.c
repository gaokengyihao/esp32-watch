#include "ota.h"
#include "ble_uart.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define OTA_MAGIC       0x4F54
#define OTA_CMD_START   1
#define OTA_CMD_DATA    2
#define OTA_CMD_END     3
#define OTA_CMD_REBOOT  4
#define OTA_CMD_ABORT   5
#define OTA_ACK_FLAG    0x80

typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint8_t  cmd;
    uint8_t  seq;
    uint16_t len;
    uint32_t offset;
    uint32_t crc;
} ota_hdr_t;

typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint8_t  cmd;
    uint8_t  status;
    uint32_t offset;
} ota_ack_t;

static const char *TAG = "ota";
static esp_ota_handle_t s_ota;
static const esp_partition_t *s_part;
static uint32_t s_size;
static uint32_t s_written;
static bool s_active;

static void send_ack(uint8_t cmd, uint8_t status, uint32_t offset)
{
    ota_ack_t a = {
        .magic  = OTA_MAGIC,
        .cmd    = (uint8_t)(cmd | OTA_ACK_FLAG),
        .status = status,
        .offset = offset,
    };
    ble_uart_tx((const uint8_t *)&a, sizeof(a));
}

void ota_handle_frame(const uint8_t *data, size_t len)
{
    if (len < sizeof(ota_hdr_t)) {
        return;
    }

    ota_hdr_t h;
    memcpy(&h, data, sizeof(h));
    if (h.magic != OTA_MAGIC) {
        return;
    }
    if (h.len != len - sizeof(h)) {
        send_ack(h.cmd, 1, s_written);
        return;
    }

    const uint8_t *payload = data + sizeof(h);

    switch (h.cmd) {
    case OTA_CMD_START: {
        if (h.len < 4) {
            send_ack(h.cmd, 2, 0);
            return;
        }

        uint32_t image_size;
        memcpy(&image_size, payload, sizeof(image_size));

        s_part = esp_ota_get_next_update_partition(NULL);
        if (s_part == NULL) {
            send_ack(h.cmd, 3, 0);
            return;
        }

        esp_err_t err = esp_ota_begin(s_part, image_size, &s_ota);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_begin: %s", esp_err_to_name(err));
            send_ack(h.cmd, 4, 0);
            return;
        }

        s_size    = image_size;
        s_written = 0;
        s_active  = true;

        ESP_LOGI(TAG, "start %s, size=%u", s_part->label, (unsigned)image_size);
        send_ack(h.cmd, 0, 0);
        break;
    }

    case OTA_CMD_DATA: {
        if (!s_active || h.offset != s_written) {
            send_ack(h.cmd, 5, s_written);
            return;
        }
        if (h.len == 0 || s_written + h.len > s_size) {
            send_ack(h.cmd, 6, s_written);
            return;
        }

        esp_err_t err = esp_ota_write(s_ota, payload, h.len);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write: %s", esp_err_to_name(err));
            esp_ota_abort(s_ota);
            s_active = false;
            send_ack(h.cmd, 7, s_written);
            return;
        }

        s_written += h.len;
        send_ack(h.cmd, 0, s_written);
        break;
    }

    case OTA_CMD_END: {
        if (!s_active) {
            send_ack(h.cmd, 8, s_written);
            return;
        }

        esp_err_t err = esp_ota_end(s_ota);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_end: %s", esp_err_to_name(err));
            s_active = false;
            send_ack(h.cmd, 9, s_written);
            return;
        }

        err = esp_ota_set_boot_partition(s_part);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "set_boot: %s", esp_err_to_name(err));
            s_active = false;
            send_ack(h.cmd, 10, s_written);
            return;
        }

        s_active = false;
        ESP_LOGI(TAG, "end, boot partition set to %s", s_part->label);
        send_ack(h.cmd, 0, s_written);
        break;
    }

    case OTA_CMD_REBOOT: {
        send_ack(h.cmd, 0, s_written);
        vTaskDelay(pdMS_TO_TICKS(300));
        esp_restart();
        break;
    }

    case OTA_CMD_ABORT: {
        if (s_active) {
            esp_ota_abort(s_ota);
            s_active = false;
        }
        send_ack(h.cmd, 0, 0);
        break;
    }

    default:
        send_ack(h.cmd, 11, s_written);
        break;
    }
}

void ota_mark_valid(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (running) {
        ESP_LOGI(TAG, "running partition: %s", running->label);
    }

    esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "app marked valid");
    } else if (err != ESP_ERR_OTA_ROLLBACK_INVALID_STATE) {
        ESP_LOGW(TAG, "mark valid: %s", esp_err_to_name(err));
    }
}