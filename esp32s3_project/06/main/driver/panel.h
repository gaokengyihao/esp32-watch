#ifndef     _PANEL_H_
#define     _PANEL_H_

#include <stdint.h>

typedef const struct 
{
    uint16_t TFT_H;
    uint16_t TFT_W;
}tft_sizetypedef;

#define atk_md0240 1
#define p183b001_v4_ctp 0

/*tft屏参数*/
#if atk_md0240
#define TFT_H   240   
#define TFT_W   284
#elif p183b001_v4_ctp
#define TFT_H   240   
#define TFT_W   284
#endif

#endif