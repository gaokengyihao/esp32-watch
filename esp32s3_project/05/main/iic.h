#ifndef     _IIC_H_
#define     _IIC_H_

#define     tft_iic_scl_bit     9
#define     tft_iic_sda_bit     8

#define     tft_iic_scl     GPIO_NUM_9
#define     tft_iic_sda     GPIO_NUM_8


void tft_iic_gpio_init();
void iic_start();
void iic_stop();
uint8_t iic_write_byte(uint8_t byte);
uint8_t iic_read_byte(uint8_t ack);

#endif