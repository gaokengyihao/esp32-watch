#include "sys.h"

static uint8_t draw_buf[240 * 40 * 2];

/*返回上电后运行的时间 单位：ms*/
static uint32_t my_tick_get_cb()
{
  return (uint32_t)(esp_timer_get_time() / 1000);  
}

void my_lv_display_flush_cb_t(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
  tft_range_set(area->x1,area->x2,area->y1,area->y2);
  
  uint16_t *p = (uint16_t *)px_map;
  uint32_t count = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);
  for (uint32_t i = 0; i < count; i++)
  {
      tft_color_set(p[i]);
  }
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
    lv_display_t * disp = lv_display_create(240, 284);//水平像素240，垂直像素284
    lv_display_set_buffers(disp, draw_buf, 0, sizeof(draw_buf),LV_DISPLAY_RENDER_MODE_PARTIAL);

    // 其像素格式和尺寸(creating_a_display)；
    // 应在何处为其渲染像素(draw_buffers)；
    // 如何将那些渲染好的像素发送给它(flush_callback)。
    lv_display_set_flush_cb(disp, my_lv_display_flush_cb_t);
}

static lv_obj_t * image_create(lv_obj_t * parent,const lv_image_dsc_t * image_struct,int32_t w,int32_t h,int32_t x,int32_t y)
{
    lv_obj_t * image = lv_image_create(parent);
    lv_image_set_src(image, image_struct);

    lv_obj_set_size(image, w, h);//实际大小
    lv_obj_set_pos(image, x, y);//相对位置

    uint32_t scale_x = w * 256 / image_struct->header.w; 
    uint32_t scale_y = h * 256 / image_struct->header.h ;
    lv_image_set_scale_x(image, scale_x);
    lv_image_set_scale_y(image, scale_y);

    return image;
}

/*主页面ui显示*/
void ui_show()
{
    //初始化
    my_lvgl_port_init();

    //父类  框
    lv_obj_t * parent = lv_obj_create(lv_screen_active());//创建组件
    lv_obj_set_width(parent, lv_pct(100));//设置长
    lv_obj_set_height(parent, lv_pct(100));//设置宽
    lv_obj_set_pos(parent, 0, 0);//相对位置

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x101418), 0);  // 深色底
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);              // 不透明度=100%

    lv_obj_set_style_border_width(parent, 0, 0);//去掉边框显示
    lv_obj_set_style_radius(parent, 0, 0);//设置每个角落的半径

    //lv_obj_set_style_pad_all(parent, 0, 0);//清内边距

    //子类  框
    // lv_obj_t * obj1 = make_block(parent, 0, 0, 50, 50)

    lv_obj_t * obj1 = lv_obj_create(parent);//创建组件
    lv_obj_set_width(obj1, lv_pct(50));//设置长
    lv_obj_set_height(obj1, lv_pct(50));//设置宽
    lv_obj_set_pos(obj1, 0, 0);//相对位置

    lv_obj_set_style_bg_color(obj1, lv_color_hex(0x101418), 0);  // 深色底
    lv_obj_set_style_bg_opa(obj1, LV_OPA_COVER, 0);              // 不透明度=100%

    lv_obj_set_style_border_width(obj1, 0, 0);//去掉边框显示
    lv_obj_set_style_radius(obj1, 0, 0);//设置每个角落的半径

    // lv_obj_set_style_pad_all(obj1, 0, 0);//清内边距
    
    //obj1  成员
    // lv_obj_t * obj1_label = lv_label_create(obj1);//创建标签
    // lv_label_set_text(obj1_label, "nihao");//设置文本内容
    // lv_obj_set_pos(obj1_label, 40, 40);//相对位置

    //obj1  成员
    lv_obj_t * obj1_image = image_create(obj1,&image_Battery_level_struct,lv_pct(25),lv_pct(25),0,0);

    // lv_obj_t * obj1_image = lv_image_create(obj1);
    // lv_image_set_src(obj1_image, &image_Battery_level_struct);
    // // lv_image_set_scale(obj1_image, 32);//缩放
    // lv_image_set_scale_x(obj1_image, 32);
    // lv_image_set_scale_y(obj1_image, 32);

    // lv_obj_set_size(obj1_image, 25, 25);//实际大小
    // lv_obj_set_pos(obj1_image, 20, 20);//相对位置

}

