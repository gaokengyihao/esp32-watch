#include "sys.h"

void gpio_init()
{
    gpio_config_t gpio_struct = {0};
    gpio_struct.pin_bit_mask = (1ULL << 44) | (1ULL << 43);
    gpio_struct.mode = GPIO_MODE_OUTPUT;
    gpio_config(&gpio_struct);

    gpio_set_level(GPIO_NUM_44, 0);
    gpio_set_level(GPIO_NUM_43, 0);
}