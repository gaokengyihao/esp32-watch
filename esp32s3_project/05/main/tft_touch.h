#ifndef     _TFT_TOUCH_H_
#define     _TFT_TOUCH_H_

/*引脚*/
#define     tft_touch_rst_bit      6
#define     tft_touch_int_bit      7
#define     tft_touch_rst         GPIO_NUM_6
#define     tft_touch_int     GPIO_NUM_7

typedef struct{
    uint8_t touch_num, x_high_touch, x_low_touch, y_high_touch, y_low_touch, touch_press;
    uint16_t x_touch, y_touch;
}tft_touch_structtypedef;

void tft_touch_init();
void tft_init();
void tft_iic_read();

#endif