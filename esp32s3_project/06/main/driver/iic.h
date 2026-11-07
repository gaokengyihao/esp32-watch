#ifndef     _IIC_H_
#define     _IIC_H_

#include <stdint.h>

void iic_gpio_init();

void iic_start();
void iic_stop();
uint8_t iic_write_byte(uint8_t byte);
uint8_t iic_read_byte(uint8_t ack);

/*iic*/
#define     iic_scl_bit     42
#define     iic_sda_bit     41
#define     iic_scl         GPIO_NUM_42
#define     iic_sda         GPIO_NUM_41

#define     iic_scl_writebit(iic_scl, (x))    gpio_set_level(iic_scl, (x))
#define     iic_sda_writebit(iic_sda, (x))    gpio_set_level(iic_sda, (x))



#endif