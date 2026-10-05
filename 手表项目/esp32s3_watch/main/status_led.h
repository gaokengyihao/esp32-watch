/* status_led.h —— 板载 RGB 灯（WS2812）状态指示 */
#pragma once

#include <stdint.h>
#include "esp_err.h"

/* 初始化（驱动 GPIO48 和 GPIO38，不同批次板子接的脚不一样） */
esp_err_t status_led_init(void);

/* 设置颜色，亮度 0~255。可从任意任务调用 */
void status_led_set(uint8_t r, uint8_t g, uint8_t b);
