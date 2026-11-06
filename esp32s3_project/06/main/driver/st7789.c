#include "st7789.h"

void st7789_tft_spi_send(unsigned char type,unsigned char byte)
{
    if (type)
    {
        dc_writebit(1);
    }
    else
    {
        dc_writebit(0);
    }

    spi_sendbyte(byte);
    
}

/*tft屏 上电初始化命令*/
void st7789_tft_show_init()
{
    /*引脚初始化*/
    vTaskDelay(pdMS_TO_TICKS(50));     

    /*驱动初始化*/
    tft_spi_send(Command , 0x11);     
    vTaskDelay(pdMS_TO_TICKS(120));                

    tft_spi_send(Command , 0x36);     
    tft_spi_send(Parameter , 0x00);   

    tft_spi_send(Command , 0x3A);     
    tft_spi_send(Parameter , 0x65);  

    tft_spi_send(Command , 0x21);
    vTaskDelay(pdMS_TO_TICKS(120));

    tft_spi_send(Command , 0x29);
    vTaskDelay(pdMS_TO_TICKS(120));
}

/*设置窗口，先列后行*/
void st7789_tft_set_window(unsigned char x_start_high,unsigned char x_start_low,unsigned char x_end_high,unsigned char x_end_low,unsigned char y_start_high,unsigned char y_start_low,unsigned char y_end_high,unsigned char y_end_low)
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

void st7789_tft_color_set(uint16_t color)
{
    /*高位*/
    tft_spi_send(Parameter , (color >> 8));
    /*低位*/
    tft_spi_send(Parameter , color);
}

/*设定显示范围*/
void st7789_tft_range_set(uint16_t x_start,uint16_t x_end,uint16_t y_start,uint16_t y_end)
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
void st7789_tft_set(uint16_t x_start,uint16_t x_end,uint16_t y_start,uint16_t y_end,uint16_t color)
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