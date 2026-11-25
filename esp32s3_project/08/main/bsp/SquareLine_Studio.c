#include "lvgl.h"
#include "SquareLine_Studio.h"

#include "ui_btn.h"
#include "ui_calculator.h"
#include "ui_menu1.h"

#include "ui.h"

#include <stdio.h>

extern lv_indev_t *input_indev;//输入设备指针

typedef enum
{
    PAGE_NONE = 0,

    PAGE_MAIN = 1,
    PAGE_MENU1 = 2,
    PAGE_CALENDAR = 3,
    PAGE_CALCULATOR = 4,
    PAGE_STOPWATCH = 5,
    PAGE_CARD_PACK = 6,
    PAGE_HEART = 7,
    PAGE_COMPASS = 8,
    PAGE_MENU2 = 9,
} page_id_t;

/*手势*/
#define     next        LV_DIR_LEFT
#define     back        LV_DIR_RIGHT
#define     down        LV_DIR_BOTTOM

static void gesture_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_GESTURE)//检测到手势
    {
        lv_dir_t gesture_dir = lv_indev_get_gesture_dir(input_indev);//获取当前手势
        page_id_t page = (page_id_t)(uintptr_t)lv_event_get_user_data(e);//获取当前页面

        /*手势页面切换逻辑*/
        switch (page)
        {
            case 1:
                switch(gesture_dir)
                {
                    case next:
                        lv_indev_wait_release(input_indev);//防止进行手势切换后又进行点按切换
                        lv_screen_load_anim(ui_menu1,LV_SCREEN_LOAD_ANIM_OVER_TOP,10,0,false);
                        break;

                    default:
                        break;
                }
                break;

            case 2:
                switch(gesture_dir)
                {
                    case back:
                        lv_indev_wait_release(input_indev);
                        lv_screen_load_anim(ui_main_ui,LV_SCREEN_LOAD_ANIM_OVER_TOP,10,0,false);
                        break;

                    default:
                        break;
                }
                break;
            
            case 3:
                switch(gesture_dir)
                {
                    case back:
                        lv_indev_wait_release(input_indev);
                        lv_screen_load_anim(ui_menu1,LV_SCREEN_LOAD_ANIM_OVER_TOP,10,0,false);
                        break;

                    default:
                        break;
                }
                break;

            case 4:
                switch(gesture_dir)
                {
                    case back:
                        lv_indev_wait_release(input_indev);
                        lv_screen_load_anim(ui_menu1,LV_SCREEN_LOAD_ANIM_OVER_TOP,10,0,false);
                        break;

                    default:
                        break;
                }
                break;

            case 5:
                switch(gesture_dir)
                {
                    case back:
                        lv_indev_wait_release(input_indev);
                        lv_screen_load_anim(ui_menu1,LV_SCREEN_LOAD_ANIM_OVER_TOP,10,0,false);
                        break;

                    default:
                        break;
                }
                break;

            case 6:
                switch(gesture_dir)
                {
                    case back:
                        lv_indev_wait_release(input_indev);
                        lv_screen_load_anim(ui_menu1,LV_SCREEN_LOAD_ANIM_OVER_TOP,10,0,false);
                        break;

                    default:
                        break;
                }
                break;
            
            default:
                break;
        }
    }
}

/*手势事件、id注册*/
void gesture_init()
{
    lv_obj_add_event_cb(ui_main_ui, gesture_cb, LV_EVENT_GESTURE, (void *)(uintptr_t)PAGE_MAIN);
    lv_obj_add_event_cb(ui_menu1, gesture_cb, LV_EVENT_GESTURE, (void *)(uintptr_t)PAGE_MENU1);
    lv_obj_add_event_cb(ui_calendar_ui, gesture_cb, LV_EVENT_GESTURE, (void *)(uintptr_t)PAGE_CALENDAR);
    lv_obj_add_event_cb(ui_calculator, gesture_cb, LV_EVENT_GESTURE, (void *)(uintptr_t)PAGE_CALCULATOR);
    lv_obj_add_event_cb(ui_stopwatch_ui, gesture_cb, LV_EVENT_GESTURE, (void *)(uintptr_t)PAGE_STOPWATCH);
    lv_obj_add_event_cb(ui_card_pack, gesture_cb, LV_EVENT_GESTURE, (void *)(uintptr_t)PAGE_CARD_PACK);
}

/*总按键判断，判断是哪个模块的按键，再决定调用哪个函数*/
uint8_t btn_typejudge(uint8_t id)
{
    uint8_t btn_type = 0;

    if(id >= BTN_ID_MENU1_CALENDAR && id <= BTN_ID_MENU1_CARDPACK){btn_type = btn_type_menu1;}
    if(id >= BTN_ID_CALC_0 && id <= BTN_ID_CALC_C){btn_type = btn_type_calculator;}

    return btn_type;
}

/*lvgl按键回调函数*/
static void btn_test_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)//按键按下
    {
        btn_id_t id = (btn_id_t)(uintptr_t) lv_event_get_user_data(e);//获取当前按键
        uint8_t key_type = btn_typejudge(id);
        {
            switch (key_type)
            {
                case btn_type_menu1:
                    menu1_function(id);
                    break;

                case btn_type_calculator:
                    calculator_function(calculator_typejudge(id), id);
                    break;
                
                default:
                    break;
            }
        }

    }
}

/*按键事件、id注册*/
void btn_init()
{
    /*菜单1*/
    lv_obj_add_event_cb(ui_menu1_calendar,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_MENU1_CALENDAR);
    lv_obj_add_event_cb(ui_menu1_calculator,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_MENU1_CALCULATOR);
    lv_obj_add_event_cb(ui_menu1_stopwatch,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_MENU1_STOPWATCH);
    lv_obj_add_event_cb(ui_menu1_cardpack,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_MENU1_CARDPACK);

    /*计算器*/
    lv_obj_add_event_cb(ui_calc_0,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_CALC_0);
    lv_obj_add_event_cb(ui_calc_1,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_CALC_1);
    lv_obj_add_event_cb(ui_calc_2,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_CALC_2);
    lv_obj_add_event_cb(ui_calc_3,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_CALC_3);
    lv_obj_add_event_cb(ui_calc_4,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_CALC_4);
    lv_obj_add_event_cb(ui_calc_5,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_CALC_5);
    lv_obj_add_event_cb(ui_calc_6,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_CALC_6);
    lv_obj_add_event_cb(ui_calc_7,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_CALC_7);
    lv_obj_add_event_cb(ui_calc_8,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_CALC_8);
    lv_obj_add_event_cb(ui_calc_9,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_CALC_9);
    lv_obj_add_event_cb(ui_calc_dot,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_CALC_DOT);
    lv_obj_add_event_cb(ui_calc_add,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_CALC_ADD);
    lv_obj_add_event_cb(ui_calc_sub,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_CALC_SUB);
    lv_obj_add_event_cb(ui_calc_mul,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_CALC_MUL);
    lv_obj_add_event_cb(ui_calc_div,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_CALC_DIV);
    lv_obj_add_event_cb(ui_calc_eq,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_CALC_EQ);
    lv_obj_add_event_cb(ui_calc_C,btn_test_cb,LV_EVENT_CLICKED,(void *)(uintptr_t)BTN_ID_CALC_C);
}