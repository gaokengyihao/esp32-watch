#include "sys.h"

void app_main(void)
{
    //ws2812_gpio_init();

    //tft_spi_init();
    //tft_show_gpio_init();
    
    xTaskCreate(led_task, "led_task", 4096, NULL, 5, NULL);
    xTaskCreate(tft_task, "tft_task", 4096, NULL, 5, NULL);
    xTaskCreate(printf_task, "printf_task", 4096, NULL, 5, NULL);
}

