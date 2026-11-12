#include "driver/lvgl_port.h"
#include "image/image.h"

#include "atk_md0240.h"
#include "p183b001_v4_ctp.h"

void ui_init()
{
    /*lvgl部分*/
    my_lvgl_port_init();

    /*ui部分*/
    page1_show();
    page2_show();

    // ui_show();

}

void ui_scan()
{
    /*触摸读*/
    // p183b001_v4_ctp_read(&tft_touch_struct);

    /*定期调用它来处理lv_timer*/
    lv_timer_handler();

    /*手势滑动判断*/
    slide();

    

    
}

