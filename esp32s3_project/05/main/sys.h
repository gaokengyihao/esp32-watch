#ifndef     _SYS_H_
#define     _SYS_H_

#include <stdio.h>
#include "driver/gpio.h"//gpio
#include "freertos/FreeRTOS.h"    /* pdMS_TO_TICKS / portTICK_PERIOD_MS 这些宏在这 */
#include "freertos/task.h"        /* vTaskDelay / xTaskCreate 这些函数在这 */
#include "esp_timer.h"//esp32 

/*核心板*/
#include "gpio.h"

/*操作系统*/
#include "task.h"

/*开发板*/
#include "ws2812.h"

/*协议*/
#include "spi.h"

/*硬件*/
#include "tft.h"

/*UI lvgl*/
#include "lvgl.h"
#include "lvgl_port.h"

/*UI image*/
#include "image/image.h"

/*tft屏参数*/
#define TFT_H   284   
#define TFT_W   240 


#endif
