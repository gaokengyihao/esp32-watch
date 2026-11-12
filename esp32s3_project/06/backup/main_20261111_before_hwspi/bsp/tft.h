/*用于管理多块屏的dc、rst、pwr引脚接口*/
#ifndef     _TFT_H_
#define     _TFT_H_

#include "driver/gpio.h"

/*ATK-MD0240 tft屏引脚*/
// #define     tft_dc_bit          40
// #define     tft_dc              GPIO_NUM_40
// #define     dc_writebit(x)      gpio_set_level(tft_dc, (x))
// #define     rst_writebit(x)     xl9555_writebit(1, 2, (x))
// #define     pwr_writebit(x)     xl9555_writebit(1, 3, (x))

/*p183b001_v4_ctp tft屏引脚*/
#define     tft_dc_bit          13
#define     tft_rst_bit         14
#define     tft_dc              GPIO_NUM_13
#define     tft_rst             GPIO_NUM_14
#define     dc_writebit(x)      gpio_set_level(tft_dc, (x))
#define     rst_writebit(x)     gpio_set_level(tft_rst, (x))
#define     pwr_writebit(x)     xl9555_writebit(1, 3, (x))

#define     tft_touch_rst_bit     6
#define     tft_touch_int_bit     7
#define     tft_touch_rst         GPIO_NUM_6
#define     tft_touch_int         GPIO_NUM_7
#define     touch_rst_writebit(x)      gpio_set_level(tft_touch_rst, (x))
#define     touch_int_writebit(x)      gpio_set_level(tft_touch_int, (x))


#endif