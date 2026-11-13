#ifndef     _SYS_H_
#define     _SYS_H_

#include <stdio.h>
#include "freertos/FreeRTOS.h"    /* pdMS_TO_TICKS / portTICK_PERIOD_MS 这些宏在这 */
#include "freertos/task.h"        /* vTaskDelay / xTaskCreate 这些函数在这 */
#include "esp_timer.h"//esp32 

/*操作系统*/
#include "task.h"

/*开发板*/
#include "ws2812.h"

/*蓝牙*/
#include "bluetooth.h"

/*硬件*/
#include "tft_show.h"
#include "tft_touch.h"

/*UI lvgl*/
#include "lvgl.h"
#include "lvgl_port.h"

/*UI image*/
#include "image/image.h"

/*功能*/
#include "ota.h"//ota

/*tft屏参数*/
#define TFT_H   320   
#define TFT_W   240



#endif
