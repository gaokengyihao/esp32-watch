#include "board.h"
#include "atk_md0240.h"

/*屏参数*/
tft_sizetypedef const p183b001_v4_ctp = {
    .TFT_H = atk_md0240_h,
    .TFT_W = atk_md0240_w,
};

//dc:发命令时置0，发数据置1

/*tft屏 显示部分引脚配置*/
void atk_md0240_gpio_init()
{
    gpio_config_t gpio_struct = {0};
    gpio_struct.pin_bit_mask = (1ULL << tft_dc_bit);
    gpio_struct.mode = GPIO_MODE_OUTPUT;
    gpio_config(&gpio_struct);

    xl9555_io_mode_init(1, 2, 0);//slcd_rst
    xl9555_io_mode_init(1, 3, 0);//slcd_pwr
}

void atk_md0240_init()
{
    PWR_writebit(1);
    
    dc_writebit(0);
    rst_writebit(0);//硬件复位
    vTaskDelay(pdMS_TO_TICKS(100));
    rst_writebit(1);
    vTaskDelay(pdMS_TO_TICKS(100));
}
