#ifndef     _SPI_H_
#define     _SPI_H_

void spi_gpio_init();
void spi_sendbit(unsigned char bit);
void spi_sendbyte(unsigned char byte);

/*spi*/
#define     tft_spi_cs_bit      21
#define     spi_sda_bit         11
#define     spi_clk_bit         12
#define     tft_spi_cs          GPIO_NUM_21
#define     spi_sda             GPIO_NUM_11
#define     spi_clk             GPIO_NUM_12

#define     spi_cs_writebit(x)    gpio_set_level(tft_spi_cs, (x))
#define     spi_sda_writebit(x)    gpio_set_level(spi_sda, (x))
#define     spi_clk_writebit(x)    gpio_set_level(spi_clk, (x))

#endif
