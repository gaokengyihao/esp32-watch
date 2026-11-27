#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "esp_timer.h"

#include "ui_menu1.h"
#include "ui_btn.h"

#include "ui_stopwatch.h"

typedef struct{
    uint8_t loop_flag, min, sec, ms;
    int64_t start_time, current_time, delta, pause_start_time, pause_current_time, pause_delta;
}sw_structtypedef;

sw_structtypedef sw_struct;

void stopwatch_function(btn_id_t id)
{
    switch (id)
    {
        case BTN_ID_SW_START:
            if(!sw_struct.start_time)//第一次按下开始，记住初始值，后每一次按下为暂停和运行
            {
                sw_struct.start_time = esp_timer_get_time();
            }
            if(sw_struct.loop_flag)//当前状态为运行，则切换至暂停状态
            {
                sw_struct.pause_start_time = esp_timer_get_time();//获取暂停时间
            }
            else//当前为停止或第一次开始
            {
                if(sw_struct.pause_start_time)//停止开始时间为0，判断为第一次开始，否则判断当前状态为停止
                {
                    sw_struct.pause_current_time = esp_timer_get_time();
                    sw_struct.pause_delta = sw_struct.pause_current_time - sw_struct.pause_start_time;
                }
            }
            sw_struct.loop_flag ^= 1;
            break;
        
        case BTN_ID_SW_RESET:
            if(sw_struct.start_time)//未清除，即为开始后的运行或暂停状态
            {
                sw_struct.loop_flag = 0;
                memset(&sw_struct, 0, sizeof(sw_struct));
                sw_show();
            }
            break;

        default:
            break;
    }
}

void sw_scan()
{
    if (sw_struct.loop_flag)
    {
        sw_struct.current_time = esp_timer_get_time();
        sw_struct.current_time -= sw_struct.pause_delta;
        sw_struct.delta = sw_struct.current_time - sw_struct.start_time;

        /*数据处理*/
        sw_data_process();

        /*显示*/
        sw_show();
    }
}

/*数据处理*/
void sw_data_process()
{
    sw_struct.min = sw_struct.delta / 60000000;  
    sw_struct.sec = sw_struct.delta % 60000000 / 1000000;
    sw_struct.ms = sw_struct.delta % 60000000 % 1000000 / 10000;//ms部分为两位，所以单位应为10ms
}

/*显示*/
void sw_show()
{
    lv_label_set_text_fmt(ui_sw_min, "%02u", sw_struct.min);
    lv_label_set_text_fmt(ui_sw_sec, "%02u", sw_struct.sec);
    lv_label_set_text_fmt(ui_sw_ms, "%02u", sw_struct.ms);
}