#include "sys.h"

unsigned int number;

void led_task(void *arg)
{
    ws2812_gpio_init();
    while (1) 
    {
        WS_2812_set(0,0,255);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void printf_task(void *arg)
{
    while (1)
    {
        if(++number >= 100)
        {
            number = 0;
        }
        printf("%d\n",number);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void ui_task(void *arg)
{
    tft_spi_gpio_init();
    tft_show_gpio_init();
    tft_init();
    
    task_lvgl_init();

    while (1)
    {
        lv_timer_handler();              /* ★ 曲柄：不摇它，画面永远不动 */
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}