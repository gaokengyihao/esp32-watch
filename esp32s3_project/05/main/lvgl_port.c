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

/*主页面ui显示*/
void ui_show()
{
    my_lvgl_port_init();//初始化

    //父类  框
    lv_obj_t * parent = lv_obj_create(lv_screen_active());//创建组件
    lv_obj_set_width(parent, lv_pct(100));//设置长
    lv_obj_set_height(parent, lv_pct(100));//设置宽
    lv_obj_set_pos(parent, 0, 0);//相对位置

    //子类  框
    lv_obj_t * obj1 = lv_obj_create(parent);//创建组件
    lv_obj_set_width(obj1, lv_pct(98));//设置长
    lv_obj_set_height(obj1, lv_pct(98));//设置宽
    lv_obj_set_pos(obj1, 1, 1);//相对位置

    //obj1  成员
    lv_obj_t * obj1_label = lv_label_create(obj1);//创建标签
    lv_label_set_text(obj1_label, "nihao");//设置文本内容
    lv_obj_set_pos(obj1_label, 40, 40);//相对位置
}

