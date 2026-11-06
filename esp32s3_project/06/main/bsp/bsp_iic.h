#ifndef     _BSP_IIC_H_
#define     _BSP_IIC_H_

#include <stdint.h>

void bsp_iic_gpio_init();

void iic_start();
void iic_stop();
uint8_t iic_write_byte(uint8_t byte);
uint8_t iic_read_byte(uint8_t ack);

#endif