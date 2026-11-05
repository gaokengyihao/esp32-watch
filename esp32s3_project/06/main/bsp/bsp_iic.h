#ifndef     _BSP_IIC_H_
#define     _BSP_IIC_H_

#define     iic_scl_bit     42
#define     iic_sda_bit     41

#define     iic_scl         GPIO_NUM_42
#define     iic_sda         GPIO_NUM_41

void tft_iic_gpio_init();

void iic_start();
void iic_stop();
uint8_t iic_write_byte(uint8_t byte);
uint8_t iic_read_byte(uint8_t ack);

#endif