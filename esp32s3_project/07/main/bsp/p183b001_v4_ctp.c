#include "p183b001_v4_ctp.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h" 
#include "lvgl.h"

#include "spi.h"
#include "iic.h"
#include "st7789.h"

#include "tft.h"

/*tft屏 引脚配置*/
void p183b001_v4_ctp_gpio_init()
{
    /*显示*/
    gpio_config_t gpio_struct = {0};
    gpio_struct.pin_bit_mask = (1ULL << tft_dc_bit) | (1ULL << tft_rst_bit);
    gpio_struct.mode = GPIO_MODE_OUTPUT;
    gpio_config(&gpio_struct);

    /*触摸*/
    gpio_struct.pin_bit_mask = (1ULL << tft_touch_rst_bit) ;
    gpio_struct.mode = GPIO_MODE_OUTPUT;
    gpio_config(&gpio_struct);

    iic_gpio_init();
    spi_gpio_init();
}

/*tft屏 初始化*/
void p183b001_v4_ctp_init()
{   
    /*引脚初始化*/
    p183b001_v4_ctp_gpio_init();
    
    /*屏复位*/
    rst_writebit(0);
    vTaskDelay(pdMS_TO_TICKS(100));
    rst_writebit(1);
    vTaskDelay(pdMS_TO_TICKS(100));

    /*触摸复位*/
    touch_rst_writebit(0);
    vTaskDelay(pdMS_TO_TICKS(100));
    touch_rst_writebit(1);
    vTaskDelay(pdMS_TO_TICKS(100));

    st7789_init();
}


//地址0X15，写发0，读发1

#define address_write 0X2A
#define address_read  0X2B

tft_touch_structtypedef tft_touch_struct;

#define debug   0

/*读取并处理tft触摸数据*/
void p183b001_v4_ctp_read(tft_touch_structtypedef * tft_touch_struct)
{
    /*起始位*/
    iic_start();

    /*发从机地址+写，发要读的地址*/
#if  debug
    if(iic_write_byte(address_write))
    {
        printf("slave_address_write error\n");//从机地址错误
    }
#else
    iic_write_byte(address_write);
#endif

#if  debug
    if(iic_write_byte(0X02))
    {
        printf("register_address_write error\n");//寄存器地址错误
    }
#else
    iic_write_byte(0X02);
#endif

    /*重新起始*/
    iic_start();

    /*发从机地址+读*/
    iic_write_byte(address_read);

    /*连续读6个字节*/
    // tft_touch_struct.touch_num = iic_read_byte(1);
    // tft_touch_struct.x_high_touch = iic_read_byte(1);
    // tft_touch_struct.x_low_touch = iic_read_byte(1);
    // tft_touch_struct.y_high_touch = iic_read_byte(1);
    // tft_touch_struct.y_low_touch = iic_read_byte(1);
    // tft_touch_struct.touch_press = iic_read_byte(0);
    tft_touch_struct->touch_num = iic_read_byte(1);
    tft_touch_struct->x_high_touch = iic_read_byte(1);
    tft_touch_struct->x_low_touch = iic_read_byte(1);
    tft_touch_struct->y_high_touch = iic_read_byte(1);
    tft_touch_struct->y_low_touch = iic_read_byte(1);
    tft_touch_struct->touch_press = iic_read_byte(0);
    

    /*停止位*/
    iic_stop();

    /*数据处理*/
    tft_touch_struct->x_high_touch &= 0X0F;//高位只有低4位有效，对高4位清零
    tft_touch_struct->y_high_touch &= 0X0F;

    tft_touch_struct->x_touch = (tft_touch_struct->x_high_touch << 8) | tft_touch_struct->x_low_touch;
    tft_touch_struct->y_touch = (tft_touch_struct->y_high_touch << 8) | tft_touch_struct->y_low_touch;

#if  debug
    if(tft_touch_struct->touch_num)
    {
       printf("x = %d, y = %d\n", tft_touch_struct->x_touch, tft_touch_struct->y_touch); 
    }
#endif

}


