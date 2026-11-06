#ifndef     _BSP_SPI_H_
#define     _BSP_SPI_H_

void bsp_spi_gpio_init();
void spi_sendbit(unsigned char bit);
void spi_sendbyte(unsigned char byte);

#endif