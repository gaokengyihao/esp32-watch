#ifndef     _TFT_H_
#define     _TFT_H_

/*引脚*/
#define     tft_show_dc_bit      13
#define     tft_show_rst_bit     14
#define     tft_show_dc      GPIO_NUM_13
#define     tft_show_rst     GPIO_NUM_14

/*发送类型*/
#define     Command       0
#define     Parameter     1

/* 基础色 */
#define BLACK        0x0000   /* 黑   (0,0,0) */
#define WHITE        0xFFFF   /* 白   (255,255,255) */
#define RED          0xF800   /* 红   (255,0,0) */
#define GREEN        0x07E0   /* 绿   (0,255,0) */
#define BLUE         0x001F   /* 蓝   (0,0,255) */
#define YELLOW       0xFFE0   /* 黄   (255,255,0) */
#define CYAN         0x07FF   /* 青   (0,255,255) */
#define MAGENTA      0xF81F   /* 品红 (255,0,255) */
#define ORANGE       0xFD20   /* 橙   (255,165,0) */
#define PINK         0xFE19   /* 粉   (255,192,203) */
#define PURPLE       0x8010   /* 紫   (128,0,128) */
#define BROWN        0xA145   /* 棕   (165,42,42) */
#define SKYBLUE      0x867D   /* 天蓝 (135,206,235) */

/* 灰阶（做背景/文字很常用）*/
#define DARKGRAY     0x4208   /* 深灰 (64,64,64)   */
#define GRAY         0x8410   /* 中灰 (128,128,128) */
#define LIGHTGRAY    0xCE59   /* 浅灰 (200,200,200) */

void tft_show_gpio_init();
void tft_send(unsigned char type,unsigned char byte);
void tft_init();
void tft_set(uint16_t x_start,uint16_t x_end,uint16_t y_start,uint16_t y_end,uint16_t color);



#endif