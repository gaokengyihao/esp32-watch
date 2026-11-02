#include "sys.h"

//dc:发命令时置0，发数据置1

/*tft屏 显示部分引脚配置*/
void tft_show_gpio_init()
{
    gpio_config_t gpio_struct = {0};
    gpio_struct.pin_bit_mask = (1ULL << tft_show_dc_bit) | (1ULL << tft_show_rst_bit);
    gpio_struct.mode = GPIO_MODE_OUTPUT;
    gpio_config(&gpio_struct);

    gpio_set_level(tft_show_dc, 0);
    gpio_set_level(tft_show_rst, 0);//硬件复位
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(tft_show_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
}

void tft_spi_send(unsigned char type,unsigned char byte)
{
    if (type)
    {
        gpio_set_level(tft_show_dc, 1);
    }
    else
    {
        gpio_set_level(tft_show_dc, 0);
    }

    spi_sendbyte(byte);
    
}

/*tft屏 上电初始化命令*/
void tft_show_init()
{
    /*引脚初始化*/
    tft_spi_gpio_init();
    tft_show_gpio_init();
    vTaskDelay(pdMS_TO_TICKS(50));     

}

/*设置窗口，先列后行*/
void tft_set_window(unsigned char x_start_high,unsigned char x_start_low,unsigned char x_end_high,unsigned char x_end_low,unsigned char y_start_high,unsigned char y_start_low,unsigned char y_end_high,unsigned char y_end_low)
{
    tft_spi_send(Command , 0x2A);//列 
    tft_spi_send(Parameter , x_start_high);
    tft_spi_send(Parameter , x_start_low);
    tft_spi_send(Parameter , x_end_high);
    tft_spi_send(Parameter , x_end_low);

    tft_spi_send(Command , 0x2B);//行 
    tft_spi_send(Parameter , y_start_high);
    tft_spi_send(Parameter , y_start_low);
    tft_spi_send(Parameter , y_end_high);
    tft_spi_send(Parameter , y_end_low);
}

void tft_color_set(uint16_t color)
{
    /*高位*/
    tft_spi_send(Parameter , (color >> 8));
    /*低位*/
    tft_spi_send(Parameter , color);
}

/*设定显示范围*/
void tft_range_set(uint16_t x_start,uint16_t x_end,uint16_t y_start,uint16_t y_end)
{
    unsigned char x_start_high = (x_start >> 8);
    unsigned char x_start_low = x_start;
    unsigned char x_end_high = (x_end >> 8);
    unsigned char x_end_low = x_end;
    unsigned char y_start_high = (y_start >> 8);
    unsigned char y_start_low = y_start;
    unsigned char y_end_high = (y_end >> 8);
    unsigned char y_end_low = y_end;

    //窗口
    tft_set_window(x_start_high,x_start_low,x_end_high,x_end_low,y_start_high,y_start_low,y_end_high,y_end_low);

    //收像素
    tft_spi_send(Command , 0x2C);
}

/*测试 刷新显示*/
void tft_set(uint16_t x_start,uint16_t x_end,uint16_t y_start,uint16_t y_end,uint16_t color)
{
    unsigned char x_start_high = (x_start >> 8);
    unsigned char x_start_low = x_start;
    unsigned char x_end_high = (x_end >> 8);
    unsigned char x_end_low = x_end;
    unsigned char y_start_high = (y_start >> 8);
    unsigned char y_start_low = y_start;
    unsigned char y_end_high = (y_end >> 8);
    unsigned char y_end_low = y_end;

    //窗口
    tft_set_window(x_start_high,x_start_low,x_end_high,x_end_low,y_start_high,y_start_low,y_end_high,y_end_low);

    //收像素
    tft_spi_send(Command , 0x2C); 

    //发颜色数据，从左往右，从上往下
    uint32_t count = (x_end - x_start + 1) * (y_end - y_start + 1);
    for (uint32_t i = 0; i < count; i++)
    {
        tft_color_set(color);
    }
}
