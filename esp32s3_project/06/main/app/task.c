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

void lvgl_task(void *arg)
{
    vTaskDelay(pdMS_TO_TICKS(500));

    /*板子引脚初始化*/
    board_init();

    /*显示部分*/
    tft_show_init();

    /*触摸部分*/
    // tft_touch_init();

    /*lvgl部分*/
    my_lvgl_port_init();

    /*ui部分*/
    // ui_show();
    image_show(&gaokengstruct);

    while (1)
    {
        lv_timer_handler();
        // tft_iic_read();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void bluetooth_task(void *arg)
{
    bluetooth_init(); 
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}