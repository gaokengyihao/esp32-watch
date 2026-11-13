#include "atk_md0240.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h" 

#include "spi.h"
#include "iic.h"
#include "xl9555.h"
#include "st7789.h"

#include "tft.h"

//dc:发命令时置0，发数据置1

/*tft屏 显示部分引脚配置*/
void atk_md0240_gpio_init()
{
    gpio_config_t gpio_struct = {0};
    gpio_struct.pin_bit_mask = (1ULL << tft_dc_bit);
    gpio_struct.mode = GPIO_MODE_OUTPUT;
    gpio_config(&gpio_struct);

    spi_gpio_init();
    iic_gpio_init();        //先配置iic引脚
    xl9555struct_DeInit();  //然后对xl9555寄存器结构体初始化，最后通过xl9555更改更改引脚状态

    xl9555_io_mode_init(1, 2, 0);//slcd_rst
    xl9555_io_mode_init(1, 3, 0);//slcd_pwr
}

void atk_md0240_init()
{
    atk_md0240_gpio_init();

    pwr_writebit(1);
    
    dc_writebit(0);
    rst_writebit(0);//硬件复位
    vTaskDelay(pdMS_TO_TICKS(100));
    rst_writebit(1);
    vTaskDelay(pdMS_TO_TICKS(100));

    st7789_init();
}

