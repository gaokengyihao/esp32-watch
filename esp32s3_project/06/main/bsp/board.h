#ifndef     _BOARD_H_
#define     _BOARD_H_

/*iic*/
#define     iic_scl_bit     42
#define     iic_sda_bit     41
#define     iic_scl         GPIO_NUM_42
#define     iic_sda         GPIO_NUM_41

/*spi*/
#define     tft_spi_cs_bit      21
#define     tft_spi_sda_bit     11
#define     spi_clk_bit         12
#define     spi_clk             GPIO_NUM_12
#define     tft_spi_cs          GPIO_NUM_21
#define     tft_spi_sda         GPIO_NUM_11

/*ws2812*/
#define     ws2812              GPIO_NUM_12
#define     ws2812_writebit(x)  gpio_set_level(ws2812, x)

/*ATK-MD0240 tft屏引脚*/
#define     tft_dc_bit          40
#define     tft_dc              GPIO_NUM_40
#define     dc_writebit(x)      gpio_set_level(tft_dc, (x))
#define     rst_writebit(x)     xl9555_writebit(1, 2, (x))
#define     PWR_writebit(x)     xl9555_writebit(1, 3, (x))

/*P183B001-V4-CTP tft屏引脚*/
#define     tft_touch_rst_bit     6
#define     tft_touch_int_bit     7
#define     tft_rst_bit           14
// #define     tft_PWR_bit           14
#define     tft_touch_rst         GPIO_NUM_6
#define     tft_touch_int         GPIO_NUM_7
#define     tft_rst               GPIO_NUM_14
// #define     tft_PWR               GPIO_NUM_14

/*esp32库*/
#include "driver/gpio.h"//gpio

#include "bsp_spi.h"
#include "bsp_iic.h"
#include "bsp_xl9555.h"


void board_init();

#endif