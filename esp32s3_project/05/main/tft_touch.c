#include "sys.h"

/*tft屏 触摸部分引脚配置*/
void tft_touch_gpio_init()
{
    gpio_config_t gpio_struct = {0};
    gpio_struct.pin_bit_mask = (1ULL << tft_touch_rst_bit) ;
    gpio_struct.mode = GPIO_MODE_OUTPUT;
    gpio_config(&gpio_struct);

    gpio_config_t gpio_struct1 = {0};
    gpio_struct1.pin_bit_mask = (1ULL << tft_touch_int_bit) ;
    gpio_struct1.mode = GPIO_MODE_INPUT;
    gpio_struct1.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&gpio_struct1);
}

/*tft屏 触摸初始化*/
void tft_touch_init()
{   
    /*引脚初始化*/
    tft_iic_gpio_init();
    tft_touch_gpio_init();
    
    gpio_set_level(tft_touch_rst, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(tft_touch_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
}

//地址0X15，写发0，读发1

#define address_write 0X2A
#define address_read  0X2B

tft_touch_structtypedef tft_touch_struct;

#define debug   1

/*读取并处理tft触摸数据*/
void tft_iic_read()
{
    /*起始位*/
    iic_start();

    /*发从机地址+写，发要读的地址*/
#ifdef  debug
    if(iic_write_byte(address_write))
    {
        printf("slave_address_write error\n");//从机地址错误
    }
#else
    iic_write_byte(address_write);
#endif

#ifdef  debug
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
    tft_touch_struct.touch_num = iic_read_byte(1);
    tft_touch_struct.x_high_touch = iic_read_byte(1);
    tft_touch_struct.x_low_touch = iic_read_byte(1);
    tft_touch_struct.y_high_touch = iic_read_byte(1);
    tft_touch_struct.y_low_touch = iic_read_byte(1);
    tft_touch_struct.touch_press = iic_read_byte(0);

    /*停止位*/
    iic_stop();

    /*数据处理*/
    tft_touch_struct.x_high_touch &= 0X0F;//高位只有低4位有效，对高4位清零
    tft_touch_struct.y_high_touch &= 0X0F;

    tft_touch_struct.x_touch = (tft_touch_struct.x_high_touch << 8) | tft_touch_struct.x_low_touch;
    tft_touch_struct.y_touch = (tft_touch_struct.y_high_touch << 8) | tft_touch_struct.y_low_touch;

#ifdef  debug
    if(tft_touch_struct.touch_num)
    {
       printf("x = %d, y = %d\n", tft_touch_struct.x_touch, tft_touch_struct.y_touch); 
    }
#endif
    
}
