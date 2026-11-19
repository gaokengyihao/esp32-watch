#include "lvgl.h"
#include "lvgl_port.h"

#include "esp_timer.h"//esp32 

#include "st7789.h"
#include "p183b001_v4_ctp.h"

lv_display_t * panel_dis;//显示面板
lv_indev_t * input_indev;//输入设备指针

static uint8_t draw_buf[240 * 40 * 2] __attribute__((aligned(4)));//硬件spi+dma，dma要求源地址为4的倍数

// static uint8_t draw_buf[240 * 40 * 2];   //atk_md0240
// static uint8_t draw_buf[320 * 40 * 2];   //p183b001_v4_ctp

/*返回上电后运行的时间 单位：ms*/
static uint32_t my_tick_get_cb()
{
  return (uint32_t)(esp_timer_get_time() / 1000);  
}

/*(硬件spi)tft屏刷新回调函数*/
void my_lv_display_flush_cb_t(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    st7789_tft_range_set(area->x1,area->x2,area->y1,area->y2);
    uint32_t count = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);

    lv_draw_rgb565_swap(px_map, count);//px_map 是 LVGL 的 RGB565 小端数据，ST7789 要高位在前。

    /*数组大小*/
    uint16_t length = count * 2;
    st7789_tft_spi_sendbuf(Parameter, px_map, length);

    /*通知lvgl发送完成*/
    lv_display_flush_ready(disp);
}

/*触摸回调函数*/
void indev_read_cb_t(lv_indev_t * indev, lv_indev_data_t * data)
{
    p183b001_v4_ctp_read(&tft_touch_struct);

    if(tft_touch_struct.touch_num)//手指按下判断
    {
        data->state = LV_INDEV_STATE_PRESSED;

        data->point.x = tft_touch_struct.x_touch;
        data->point.y = tft_touch_struct.y_touch;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// 需要向 LVGL 提供有关您希望它使用的每个显示面板的相关信息。具体来说，需要提供以下信息：
// 其像素格式和尺寸(creating_a_display)；
// 应在何处为其渲染像素(draw_buffers)；
// 如何将那些渲染好的像素发送给它(flush_callback)。
void my_lvgl_port_init()
{
    lv_init();

    //为 LVGL 提供一个回调函数，以获取系统已过去的毫秒数
    lv_tick_set_cb(my_tick_get_cb);

    //内存
    panel_dis = lv_display_create(TFT_W, TFT_H);//水平像素240，垂直像素284
    lv_display_set_buffers(panel_dis, draw_buf, 0, sizeof(draw_buf),LV_DISPLAY_RENDER_MODE_PARTIAL);

    // 其像素格式和尺寸(creating_a_display)；
    // 应在何处为其渲染像素(draw_buffers)；
    // 如何将那些渲染好的像素发送给它(flush_callback)。
    lv_display_set_flush_cb(panel_dis, my_lv_display_flush_cb_t);

    /*手势判断所需*/
    input_indev = lv_indev_create();
    lv_indev_set_type(input_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(input_indev, indev_read_cb_t);
}
