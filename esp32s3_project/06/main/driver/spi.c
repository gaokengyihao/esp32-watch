#include "spi.h"

/*tft屏 spi通信引脚初始化*/
void tft_spi_gpio_init()
{
    gpio_config_t gpio_struct = {0};
    gpio_struct.pin_bit_mask = (1ULL << tft_spi_cs_bit) | (1ULL << tft_spi_sda_bit) | (1ULL << spi_clk_bit);
    gpio_struct.mode = GPIO_MODE_OUTPUT;
    gpio_config(&gpio_struct);

    spi_cs_writebit(1);//cs上拉
    spi_clk_writebit(0);//clk置0
}

/*tft屏 spi通信引脚初始化*/
void bsp_spi_gpio_init()
{
    gpio_config_t gpio_struct = {0};
    gpio_struct.pin_bit_mask = (1ULL << tft_spi_cs_bit) | (1ULL << tft_spi_sda_bit) | (1ULL << spi_clk_bit);
    gpio_struct.mode = GPIO_MODE_OUTPUT;
    gpio_config(&gpio_struct);

    spi_cs_writebit(1);//cs上拉
    spi_clk_writebit(0);//clk置0
}

void spi_sendbit(unsigned char bit)
{
    spi_sda_writebit(bit);
    spi_clk_writebit(1);
    esp_rom_delay_us(1);
    spi_clk_writebit(0);
    esp_rom_delay_us(1);
}

void spi_sendbyte(unsigned char byte)
{
    spi_cs_writebit(0);//片选

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
    
    spi_cs_writebit(1);//片选结束
}