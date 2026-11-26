#ifndef     _P183B001_V4_CTP_H_
#define     _P183B001_V4_CTP_H_

#include <stdint.h>

typedef struct{
    uint8_t touch_num, x_high_touch, x_low_touch, y_high_touch, y_low_touch, touch_press;
    uint16_t x_touch, y_touch;
}tft_touch_structtypedef;

/*触摸 Y 坐标与显示 Y 坐标的偏差补偿*/
#define CTP_TOUCH_Y_OFFSET   (-20)

extern tft_touch_structtypedef tft_touch_struct;

void p183b001_v4_ctp_gpio_init();
void p183b001_v4_ctp_init();
void p183b001_v4_ctp_read(tft_touch_structtypedef * tft_touch_struct);

#endif