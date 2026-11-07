#ifndef     _ATK_MD0240_H_
#define     _ATK_MD0240_H_

void atk_md0240_gpio_init();
void atk_md0240_init();

/*ATK-MD0240 tft屏引脚*/
#define     tft_dc_bit          40
#define     tft_dc              GPIO_NUM_40
#define     dc_writebit(x)      gpio_set_level(tft_dc, (x))
#define     rst_writebit(x)     xl9555_writebit(1, 2, (x))
#define     PWR_writebit(x)     xl9555_writebit(1, 3, (x))

/*tft屏参数*/
#define atk_md0240_h 320
#define atk_md0240_w 240

#endif