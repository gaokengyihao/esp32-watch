#ifndef     _P183B001_V4_CTP_H_
#define     _P183B001_V4_CTP_H_

#include "panel.h"

void tft_touch_init();
void tft_init();
void tft_iic_read();

extern tft_sizetypedef p183b001_v4_ctp;

/*P183B001-V4-CTP tft屏引脚*/
#define     tft_touch_rst_bit     6
#define     tft_touch_int_bit     7

#define     tft_touch_rst         GPIO_NUM_6
#define     tft_touch_int         GPIO_NUM_7

// #define     tft_rst_bit           14
// #define     tft_PWR_bit           14
// #define     tft_rst               GPIO_NUM_14
// #define     tft_PWR               GPIO_NUM_14

#define     tft_dc_bit          40
#define     tft_dc              GPIO_NUM_40
#define     dc_writebit(x)      gpio_set_level(tft_dc, (x))
#define     rst_writebit(x)     xl9555_writebit(1, 2, (x))
#define     PWR_writebit(x)     xl9555_writebit(1, 3, (x))

/*tft屏参数*/
#define p183b001_v4_ctp_h 284
#define p183b001_v4_ctp_w 240

#endif