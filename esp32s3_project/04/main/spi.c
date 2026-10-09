#include "sys.h"

void tft_spi_init()
{
    gpio_config_t gpio_struct;
    gpio_struct.pin_bit_mask = (1ULL << tft_spi_cs_bit) | (1ULL << tft_spi_sda_bit) | (1ULL << tft_spi_clk_bit);
    gpio_struct.mode = GPIO_MODE_OUTPUT;
    gpio_config(&gpio_struct);

    gpio_set_level(tft_spi_cs, 1);//cs上拉
    gpio_set_level(tft_spi_clk, 0);//clk置0
}

void spi_sendbit(unsigned char bit)
{
    //gpio_set_level(tft_spi_cs, 0);//片选
    gpio_set_level(tft_spi_sda, bit);//输出电平确定
    gpio_set_level(tft_spi_clk, 1);//时钟信号来
    gpio_set_level(tft_spi_clk, 0);//时钟信号结束
}

void spi_sendbyte(unsigned char byte)
{
    gpio_set_level(tft_spi_cs, 0);//片选

    for (unsigned char i = 0; i < 8; i++)
    {
        if (byte & (0x80 >> i))
        {
            spi_sendbit(1);
        }
        else
        {
            spi_sendbit(0);
        }
    }
    
    gpio_set_level(tft_spi_cs, 1);//片选结束
}