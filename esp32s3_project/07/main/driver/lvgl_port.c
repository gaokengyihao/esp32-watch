#include "lvgl.h"
#include "lvgl_port.h"

#include "esp_timer.h"//esp32 

#include "image/image.h"
#include "st7789.h"
#include "p183b001_v4_ctp.h"

static uint8_t draw_buf[240 * 40 * 2] __attribute__((aligned(4)));//硬件spi+dma，dma要求源地址为4的倍数
// static uint8_t draw_buf[240 * 40 * 2];      //atk_md0240

// static uint8_t draw_buf[320 * 40 * 2];   //p183b001_v4_ctp

lv_indev_t * input_indev;//输入设备指针
lv_display_t * panel_dis;//显示面板

lv_obj_t * page1;//页面1
lv_obj_t * page2;//页面2
lv_obj_t * page3;//页面3

lv_obj_t * calendar;//日历

/*返回上电后运行的时间 单位：ms*/
static uint32_t my_tick_get_cb()
{
  return (uint32_t)(esp_timer_get_time() / 1000);  
}

/*软件spi*/
// void my_lv_display_flush_cb_t(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
// {
//     st7789_tft_range_set(area->x1,area->x2,area->y1,area->y2);

//     uint16_t *p = (uint16_t *)px_map;
//     uint32_t count = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);
//     for (uint32_t i = 0; i < count; i++)
//     {
//         st7789_tft_color_set(p[i]);
//     }
//     lv_display_flush_ready(disp);
// }

/*硬件spi*/
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

    input_indev = lv_indev_create();
    lv_indev_set_type(input_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(input_indev, indev_read_cb_t);
}

static lv_obj_t * obj_create(lv_obj_t * parent,int32_t w,int32_t h,int32_t x,int32_t y)
{
    lv_obj_t * obj = lv_obj_create(parent);//创建组件
    lv_obj_set_size(obj, lv_pct(w), lv_pct(h));//设置长宽
    lv_obj_set_pos(obj, x, y);//相对位置

    lv_obj_set_style_bg_color(obj, lv_color_hex(0x101418), 0);  // 深色底
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);              // 不透明度=100%

    lv_obj_set_style_border_width(obj, 0, 0);//去掉边框显示
    lv_obj_set_style_radius(obj, 0, 0);//设置每个角落的半径

    lv_obj_set_scrollable(obj, false);//去掉滑动功能

    return obj;
}

static lv_obj_t * image_create(lv_obj_t * parent,const lv_image_dsc_t * image_struct,int32_t w,int32_t h,int32_t x,int32_t y)
{
    lv_obj_t * image = lv_image_create(parent);
    lv_image_set_src(image, image_struct);

    lv_obj_set_size(image, lv_pct(w), lv_pct(h));//长宽对于父类的百分比

    lv_obj_set_pos(image, x, y);//相对位置

    lv_image_set_inner_align(image, LV_IMAGE_ALIGN_STRETCH);//图片自动缩放到框的大小
    
    lv_obj_set_scrollable(image, false);//去掉滑动功能

    return image;
}

static lv_obj_t * label_create(lv_obj_t * parent,char * text)
{
    lv_obj_t * label = lv_label_create(parent);//创建标签
    lv_label_set_text(label, text);//设置文本内容
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);                       // 子对象竖着排队
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START,                       // 主轴：从顶部开始
                                LV_FLEX_ALIGN_CENTER,                      // 交叉轴：水平居中
                                LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(parent, 6, 0); 

    lv_obj_set_scrollable(label, false);//去掉滑动功能

    /*字体颜色*/
    lv_obj_set_style_text_color(label, lv_color_hex(0xE6EDF7), 0);
    
    return label;
}

/*全屏图片显示*/
void image_show(lv_obj_t * parent,const lv_image_dsc_t *imagestruct)
{
    //父类  框
    lv_obj_t * obj = obj_create(parent, 100, 100, 0, 0);

    //图片  成员
    lv_obj_t * obj1_image = image_create(obj,imagestruct,100,100,0,0);
    // lv_obj_t * obj1_label = label_create(obj1,"gaokeng");
}

void page1_show()
{   
    page1 = lv_obj_create(NULL);//创建页面
    
    //子类  框
    lv_obj_t * parent = obj_create(page1, 100, 100, 0, 0);

    //子类  框1
    lv_obj_t * obj1 = obj_create(parent, 50, 50, 0, 0);
    
    //框1  成员
    lv_obj_t * obj1_image = image_create(obj1,&image_Battery_level_struct,100,50,0,0);
    lv_obj_t * obj1_label = label_create(obj1,"Battery");

    //子类  框2
    lv_obj_t * obj2 = obj_create(parent, 50, 50, 120, 0);

    //框2   成员
    lv_obj_t * obj2_label_1 = label_create(obj2,"Thursday");
    lv_obj_t * obj2_label_2 = label_create(obj2,"2026.10.22");

    //子类  框3
    lv_obj_t * obj3 = obj_create(parent, 50, 50, 0, 141);

    //框3   成员
    lv_obj_t * button1 = lv_button_create(page1);
    lv_obj_set_size(button1, 80, 50);
    lv_obj_set_pos(button1, 80, 210);
    lv_obj_add_event_cb(button1, btn_test_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_label = lv_label_create(button1);
    lv_label_set_text(btn_label, "calendar");
    lv_obj_center(btn_label);

    //子类  框4
    lv_obj_t * obj4 = obj_create(parent, 50, 50, 120, 141);

    lv_obj_set_scrollable(page1, false);//去掉滑动功能

    /*加载页面，默认上电页面*/
    lv_screen_load(page1);
}

void page2_show()
{
    page2 = lv_obj_create(NULL);//创建页面

    image_show(page2,&gaokengstruct);

    lv_obj_set_scrollable(page2, false);//去掉滑动功能
}

void page3_show()
{
    page3 = lv_obj_create(NULL);//创建页面

    calendar = lv_calendar_add_header_dropdown(page3);
    lv_obj_set_size(calendar, 80, 50);
    lv_obj_set_pos(calendar, 0, 0);

    lv_obj_set_scrollable(page3, false);//去掉滑动功能
}

/*主页面ui显示*/
void ui_show()
{
    //父类  框
    lv_obj_t * parent = obj_create(lv_screen_active(), 100, 100, 0, 0);

    //子类  框1
    lv_obj_t * obj1 = obj_create(parent, 50, 50, 0, 0);
    
    //框1  成员
    lv_obj_t * obj1_image = image_create(obj1,&image_Battery_level_struct,100,50,0,0);
    lv_obj_t * obj1_label = label_create(obj1,"Battery");

    //子类  框2
    lv_obj_t * obj2 = obj_create(parent, 50, 50, 120, 0);

    //框2   成员
    lv_obj_t * obj2_label_1 = label_create(obj2,"Thursday");
    lv_obj_t * obj2_label_2 = label_create(obj2,"2026.10.22");

    //子类  框3
    lv_obj_t * obj3 = obj_create(parent, 50, 50, 0, 141);

    //子类  框4
    lv_obj_t * obj4 = obj_create(parent, 50, 50, 120, 141);

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

#define     next        LV_DIR_LEFT
#define     back        LV_DIR_RIGHT

void slide()
{
    lv_obj_t * now = lv_screen_active();

    switch(lv_indev_get_gesture_dir(input_indev))
    {
        case next:
            if(now != page1)
            {
                lv_screen_load_anim(page1,LV_SCREEN_LOAD_ANIM_OVER_LEFT,10,0,false);
            }
            break;

        case back:
            if(now != page2)
            {
                lv_screen_load_anim(page2,LV_SCREEN_LOAD_ANIM_OVER_RIGHT,10,0,false);
            }
            break;

        default:
            break;
    }
}

/*lvgl按键回调函数*/
static void btn_test_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        printf("button clicked!\n");
        lv_screen_load_anim(page3,LV_SCREEN_LOAD_ANIM_OVER_TOP,10,0,false);
    }
}
