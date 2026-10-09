#ifndef     _SPI_H_
#define     _SPI_H_

#define     tft_spi_cs_bit      10
#define     tft_spi_sda_bit     11
#define     tft_spi_clk_bit     12

#define     tft_spi_cs      GPIO_NUM_10
#define     tft_spi_sda     GPIO_NUM_11
#define     tft_spi_clk     GPIO_NUM_12

void tft_spi_init();
void spi_sendbit(unsigned char bit);
void spi_sendbyte(unsigned char byte);

#endif