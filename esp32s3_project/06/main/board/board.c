#include "board.h"

void board_init()
{
    iic_gpio_init();
    spi_gpio_init();
}