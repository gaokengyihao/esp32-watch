#include "sys.h"

void app_main(void)
{
    ws2812_gpio_init();
    
    while (1) 
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        WS_2812_set(255,0,0);
    }
}
