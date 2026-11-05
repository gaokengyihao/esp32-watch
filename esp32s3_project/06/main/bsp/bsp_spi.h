#ifndef     _BSP_SPI_H_
#define     _BSP_SPI_H_


#define     spi_clk_bit     12
#define     spi_clk         GPIO_NUM_12

/*tft*/
#define     tft_spi_cs_bit      21
#define     tft_spi_sda_bit     11

#define     tft_spi_cs          GPIO_NUM_21
#define     tft_spi_sda         GPIO_NUM_11

void tft_spi_gpio_init();
void spi_sendbit(unsigned char bit);
void spi_sendbyte(unsigned char byte);

#endif