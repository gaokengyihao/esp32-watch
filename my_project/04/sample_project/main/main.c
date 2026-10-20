#include "sys.h"

void app_main(void)
{
    ws2812_gpio_init();
    
    while (1) 
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        set_ws2812(0);
        set_ws2812(1);
        ws2812_ms_delay(10);
        set_ws2812(0);
    }
}
