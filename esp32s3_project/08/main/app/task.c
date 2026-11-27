#include "freertos/FreeRTOS.h"    /* pdMS_TO_TICKS / portTICK_PERIOD_MS 这些宏在这 */
#include "freertos/task.h"  
#include "lvgl.h"

#include "atk_md0240.h"
#include "p183b001_v4_ctp.h"
#include "lvgl_port.h"

#include "ws2812.h"
#include "bluetooth.h"

#include "ui.h"
#include "SquareLine_Studio.h"
#include "ui_stopwatch.h"

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

    /*tft*/
    // atk_md0240_init();
    p183b001_v4_ctp_init();

    /*ui*/
    my_lvgl_port_init();
    ui_init();//
    gesture_init();//手势
    btn_init();//按键

    while (1)
    {
        /*定期调用它来处理lv_timer*/
        lv_timer_handler();
        
        /*秒表功能轮询，需置标志位打开*/
        sw_scan();

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