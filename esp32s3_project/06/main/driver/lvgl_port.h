#ifndef     _LVGL_PORT_H_
#define     _LVGL_PORT_H_

#include "lvgl.h"

void ui_show();
void task_lvgl_init();
void my_lvgl_port_init();
void image_show(lv_obj_t * parent,const lv_image_dsc_t *imagestruct);
void page1_show();
void page2_show();
void slide();
void indev_read_cb_t(lv_indev_t * indev, lv_indev_data_t * data);

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