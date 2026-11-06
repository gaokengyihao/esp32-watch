#include "board.h"

void board_init()
{
    bsp_iic_gpio_init();
    bsp_spi_gpio_init();
    xl9555struct_DeInit();
}