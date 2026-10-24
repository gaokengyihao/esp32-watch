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

void task_lvgl_init()
{
    my_lvgl_port_init();                    /* ★ 启动 LVGL（四零件在这装好） */
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "gaokengyihao !");
    lv_obj_center(label);                /* 第一个界面元素：居中一行字 */
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

    return obj;
}

static lv_obj_t * image_create(lv_obj_t * parent,const lv_image_dsc_t * image_struct,int32_t w,int32_t h,int32_t x,int32_t y)
{
    lv_obj_t * image = lv_image_create(parent);
    lv_image_set_src(image, image_struct);

    lv_obj_set_size(image, lv_pct(w), lv_pct(h));//长宽对于父类的百分比

    lv_obj_set_pos(image, x, y);//相对位置

    lv_image_set_inner_align(image, LV_IMAGE_ALIGN_STRETCH);//图片自动缩放到框的大小
    
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

    /*字体颜色*/
    lv_obj_set_style_text_color(label, lv_color_hex(0xE6EDF7), 0);

    return label;
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

