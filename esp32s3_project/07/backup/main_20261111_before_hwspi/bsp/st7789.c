#include "st7789.h"
#include "freertos/FreeRTOS.h" 
#include "spi.h"

#include "tft.h"

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
void st7789_init()
{
    /*引脚初始化*/
    vTaskDelay(pdMS_TO_TICKS(50));     

    // /*驱动初始化*/
    // st7789_tft_spi_send(Command , 0x11);     
    // vTaskDelay(pdMS_TO_TICKS(120));                

    // st7789_tft_spi_send(Command , 0x36);     
    // st7789_tft_spi_send(Parameter , 0x60);

    // st7789_tft_spi_send(Command , 0x3A);     
    // st7789_tft_spi_send(Parameter , 0x65);  

    // st7789_tft_spi_send(Command , 0x21);
    // vTaskDelay(pdMS_TO_TICKS(120));

    // st7789_tft_spi_send(Command , 0x29);
    // vTaskDelay(pdMS_TO_TICKS(120));

    /*p183屏驱动初始化*/
    st7789_tft_spi_send(Command , 0x11);     

    vTaskDelay(pdMS_TO_TICKS(120));                

    st7789_tft_spi_send(Command , 0x36);     
    st7789_tft_spi_send(Parameter , 0x00);   

    st7789_tft_spi_send(Command , 0x3A);     
    st7789_tft_spi_send(Parameter , 0x05);   

    st7789_tft_spi_send(Command , 0xB2);     
    st7789_tft_spi_send(Parameter , 0x0C);   
    st7789_tft_spi_send(Parameter , 0x0C);   
    st7789_tft_spi_send(Parameter , 0x00);   
    st7789_tft_spi_send(Parameter , 0x33);   
    st7789_tft_spi_send(Parameter , 0x33);   

    st7789_tft_spi_send(Command , 0xB7);     
    st7789_tft_spi_send(Parameter , 0x05);   

    st7789_tft_spi_send(Command , 0xBB);     
    st7789_tft_spi_send(Parameter , 0x21);   

    st7789_tft_spi_send(Command , 0xC0);     
    st7789_tft_spi_send(Parameter , 0x2C);   

    st7789_tft_spi_send(Command , 0xC2);     
    st7789_tft_spi_send(Parameter , 0x01);   

    st7789_tft_spi_send(Command , 0xC3);     
    st7789_tft_spi_send(Parameter , 0x15);   

    st7789_tft_spi_send(Command , 0xC6);     
    st7789_tft_spi_send(Parameter , 0x0F);   

    st7789_tft_spi_send(Command , 0xD0);     
    st7789_tft_spi_send(Parameter , 0xA7);

    st7789_tft_spi_send(Command , 0xD0);     
    st7789_tft_spi_send(Parameter , 0xA4);   
    st7789_tft_spi_send(Parameter , 0xA1);   

    st7789_tft_spi_send(Command , 0xD6);     
    st7789_tft_spi_send(Parameter , 0xA1);   //sleep in后，gate输出为GND

    st7789_tft_spi_send(Command , 0xE0);
    st7789_tft_spi_send(Parameter , 0xF0);
    st7789_tft_spi_send(Parameter , 0x05);
    st7789_tft_spi_send(Parameter , 0x0E);
    st7789_tft_spi_send(Parameter , 0x08);
    st7789_tft_spi_send(Parameter , 0x0A);
    st7789_tft_spi_send(Parameter , 0x17);
    st7789_tft_spi_send(Parameter , 0x39);
    st7789_tft_spi_send(Parameter , 0x54);
    st7789_tft_spi_send(Parameter , 0x4E);
    st7789_tft_spi_send(Parameter , 0x37);
    st7789_tft_spi_send(Parameter , 0x12);
    st7789_tft_spi_send(Parameter , 0x12);
    st7789_tft_spi_send(Parameter , 0x31);
    st7789_tft_spi_send(Parameter , 0x37);

    st7789_tft_spi_send(Command , 0xE1);
    st7789_tft_spi_send(Parameter , 0xF0);
    st7789_tft_spi_send(Parameter , 0x10);
    st7789_tft_spi_send(Parameter , 0x14);
    st7789_tft_spi_send(Parameter , 0x0D);
    st7789_tft_spi_send(Parameter , 0x0B);
    st7789_tft_spi_send(Parameter , 0x05);
    st7789_tft_spi_send(Parameter , 0x39);
    st7789_tft_spi_send(Parameter , 0x44);
    st7789_tft_spi_send(Parameter , 0x4D);
    st7789_tft_spi_send(Parameter , 0x38);
    st7789_tft_spi_send(Parameter , 0x14);
    st7789_tft_spi_send(Parameter , 0x14);
    st7789_tft_spi_send(Parameter , 0x2E);
    st7789_tft_spi_send(Parameter , 0x35);

    st7789_tft_spi_send(Command , 0xE4);     
    st7789_tft_spi_send(Parameter , 0x23);   
    st7789_tft_spi_send(Parameter , 0x00);   //设定gate起点位置
    st7789_tft_spi_send(Parameter , 0x00);   //当gate没有用完时，bit4(TMG)设为0

    st7789_tft_spi_send(Command , 0x21);     

    st7789_tft_spi_send(Command , 0x29);     

    st7789_tft_spi_send(Command , 0x2C);     

}

/*设置窗口，先列后行*/
void st7789_tft_set_window(unsigned char x_start_high,unsigned char x_start_low,unsigned char x_end_high,unsigned char x_end_low,unsigned char y_start_high,unsigned char y_start_low,unsigned char y_end_high,unsigned char y_end_low)
{
    st7789_tft_spi_send(Command , 0x2A);//列 
    st7789_tft_spi_send(Parameter , x_start_high);
    st7789_tft_spi_send(Parameter , x_start_low);
    st7789_tft_spi_send(Parameter , x_end_high);
    st7789_tft_spi_send(Parameter , x_end_low);

    st7789_tft_spi_send(Command , 0x2B);//行 
    st7789_tft_spi_send(Parameter , y_start_high);
    st7789_tft_spi_send(Parameter , y_start_low);
    st7789_tft_spi_send(Parameter , y_end_high);
    st7789_tft_spi_send(Parameter , y_end_low);
}

void st7789_tft_color_set(uint16_t color)
{
    /*高位*/
    st7789_tft_spi_send(Parameter , (color >> 8));
    /*低位*/
    st7789_tft_spi_send(Parameter , color);
}

/*设定显示范围*/
void st7789_tft_range_set(uint16_t x_start,uint16_t x_end,uint16_t y_start,uint16_t y_end)
{
    y_start += TFT_Y_OFFSET;
    y_end += TFT_Y_OFFSET;

    unsigned char x_start_high = (x_start >> 8);
    unsigned char x_start_low = x_start;
    unsigned char x_end_high = (x_end >> 8);
    unsigned char x_end_low = x_end;
    unsigned char y_start_high = (y_start >> 8);
    unsigned char y_start_low = y_start;
    unsigned char y_end_high = (y_end >> 8);
    unsigned char y_end_low = y_end;

    //窗口
    st7789_tft_set_window(x_start_high,x_start_low,x_end_high,x_end_low,y_start_high,y_start_low,y_end_high,y_end_low);

    //收像素
    st7789_tft_spi_send(Command , 0x2C);
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
    st7789_tft_set_window(x_start_high,x_start_low,x_end_high,x_end_low,y_start_high,y_start_low,y_end_high,y_end_low);

    //收像素
    st7789_tft_spi_send(Command , 0x2C); 

    //发颜色数据，从左往右，从上往下
    uint32_t count = (x_end - x_start + 1) * (y_end - y_start + 1);
    for (uint32_t i = 0; i < count; i++)
    {
        st7789_tft_color_set(color);
    }
}