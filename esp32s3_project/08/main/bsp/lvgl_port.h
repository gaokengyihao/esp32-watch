#ifndef     _LVGL_PORT_H_
#define     _LVGL_PORT_H_

void my_lvgl_port_init();

#define atk_md0240 0
#define p183b001_v4_ctp 1

/*tft屏参数*/
#if atk_md0240
#define TFT_H   240   
#define TFT_W   320
#elif p183b001_v4_ctp
#define TFT_H   280
#define TFT_W   240  
#endif

#endif