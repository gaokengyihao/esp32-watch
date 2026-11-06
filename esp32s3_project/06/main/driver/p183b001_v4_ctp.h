#ifndef     _P183B001_V4_CTP_H_
#define     _P183B001_V4_CTP_H_

#include "panel.h"

void tft_touch_init();
void tft_init();
void tft_iic_read();

extern tft_sizetypedef p183b001_v4_ctp;

/*tft屏参数*/
#define p183b001_v4_ctp_h 284
#define p183b001_v4_ctp_w 240

#endif