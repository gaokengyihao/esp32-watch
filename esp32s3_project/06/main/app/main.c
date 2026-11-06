#include "freertos/FreeRTOS.h"    /* pdMS_TO_TICKS / portTICK_PERIOD_MS 这些宏在这 */
#include "freertos/task.h"        /* vTaskDelay / xTaskCreate 这些函数在这 */

void app_main(void)
{
    //ws2812_gpio_init();

    xTaskCreate(lvgl_task, "lvgl_task", 16384, NULL, 5, NULL);
    // xTaskCreate(led_task, "led_task", 4096, NULL, 5, NULL);
    // xTaskCreate(bluetooth_task, "bluetooth_task", 4096, NULL, 5, NULL);
    // xTaskCreate(printf_task, "printf_task", 4096, NULL, 5, NULL);
}

