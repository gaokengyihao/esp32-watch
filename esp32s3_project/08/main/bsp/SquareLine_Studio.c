#include "lvgl.h"
#include "SquareLine_Studio.h"

#include "ui.h"

#include <stdio.h>

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

/*按键id枚举定义*/
typedef enum
{
    BTN_ID_NONE = 0,

    /* menu1 应用菜单 */
    BTN_ID_MENU1_CALENDAR = 1,
    BTN_ID_MENU1_CALCULATOR = 2,
    BTN_ID_MENU1_STOPWATCH = 3,
    BTN_ID_MENU1_CARDPACK = 4,

    /* 秒表 */
    BTN_ID_SW_START = 5,
    BTN_ID_SW_RESET = 6,

    /* 卡包 */
    BTN_ID_CARD1_WRITE = 7,
    BTN_ID_CARD1_CHOOSE = 8,
    BTN_ID_CARD2_WRITE = 9,
    BTN_ID_CARD2_CHOOSE = 10,

    /* 设置页 menu2 */
    BTN_ID_MENU2_AOD = 11,
    BTN_ID_MENU2_OFF = 12,
    BTN_ID_MENU2_DATETIME = 13,
    BTN_ID_MENU2_WRIST = 14,
    BTN_ID_MENU2_PASSWORD = 15,

    /* 下拉栏 */
    BTN_ID_DROPDOWN_NFC = 16,
    BTN_ID_DROPDOWN_BLE = 17,
    BTN_ID_DROPDOWN_POWER = 18,
    BTN_ID_DROPDOWN_SET = 19,

    /* 主界面 */
    BTN_ID_MAIN_PULL_HANDLE = 20,
    BTN_ID_MAIN_TOP_HOT = 21,

    /* 计算器 */
    BTN_ID_CALC_0 = 22,
    BTN_ID_CALC_1 = 23,
    BTN_ID_CALC_2 = 24,
    BTN_ID_CALC_3 = 25,
    BTN_ID_CALC_4 = 26,
    BTN_ID_CALC_5 = 27,
    BTN_ID_CALC_6 = 28,
    BTN_ID_CALC_7 = 29,
    BTN_ID_CALC_8 = 30,
    BTN_ID_CALC_9 = 31,
    BTN_ID_CALC_DOT = 32,
    BTN_ID_CALC_ADD = 33,
    BTN_ID_CALC_SUB = 34,
    BTN_ID_CALC_MUL = 35,
    BTN_ID_CALC_DIV = 36,
    BTN_ID_CALC_EQ = 37,
    BTN_ID_CALC_C = 38,
} btn_id_t;

typedef struct 
{
    double calculated_value, calculated_currentvalue, calculated_tempvalue;
    uint8_t Operator, last_keynum, keynum_count;//第一次上电后赋值
}calculatorstructtypedef;

calculatorstructtypedef calculatorstruct;

char buf[32], currenbuf[64], tempbuf[32], expressionbuf[32], Operatorbuf[8];

extern lv_indev_t *input_indev;//输入设备指针

static void btn_test_cb(lv_event_t *e);//按键事件回调
static void gesture_cb(lv_event_t *e);//手势事件回调

void menu1_function(btn_id_t id);
void calculator_function(btn_id_t id);

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

typedef enum{
    /*计算器*/
    btn_type_calculator_num = 1,//计算器数字
    btn_type_calculator_Operator = 3,//计算器运算符

    /*菜单1*/
    btn_type_menu1 = 4
}btn_type_t;

/*按键类型判断*/
uint8_t calculator_typejudge(uint8_t id)
{
    uint8_t btn_type = 0;
    /*计算器*/
    if(id >= BTN_ID_CALC_0 && id <= BTN_ID_CALC_9){btn_type = btn_type_calculator_num;}
    // if(id == BTN_ID_CALC_DOT){btn_type = 2;}
    if(id >= BTN_ID_CALC_ADD && id <= BTN_ID_CALC_EQ){btn_type = btn_type_calculator_Operator;}

    /*菜单1*/
    if(id >= BTN_ID_MENU1_CALENDAR && id <= BTN_ID_MENU1_CARDPACK){btn_type = btn_type_menu1;}

    return btn_type;
}

// calculated_value, calculated_currentvalue, calculated_tempvalue
/*计算器显示*/
void calculator_show()
{
    if(calculatorstruct.Operator)
    {
        snprintf(currenbuf, sizeof(currenbuf), "%.6g", calculatorstruct.calculated_currentvalue);
        snprintf(Operatorbuf, sizeof(Operatorbuf), "%.6g", calculatorstruct.Operator);
        snprintf(tempbuf, sizeof(tempbuf), "%.6g", calculatorstruct.calculated_tempvalue);

        snprintf(expressionbuf, sizeof(expressionbuf), "%s %s %s ", tempbuf, Operatorbuf, tempbuf);
        lv_label_set_text(ui_calc_expression, expressionbuf);
    }
    else
    {
        lv_label_set_text(ui_calc_expression, "");
    }


    snprintf(buf, sizeof(buf), "%.6g", calculatorstruct.calculated_value);
    lv_label_set_text(ui_calc_value, buf);
}

/*lvgl按键回调函数*/
static void btn_test_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)//按键按下
    {
        btn_id_t id = (btn_id_t)(uintptr_t) lv_event_get_user_data(e);//获取当前按键
        uint8_t key_type = calculator_typejudge(id);
        {
            switch (key_type)
            {
                case btn_type_calculator_num:
                    calculator_function(id);
                    calculator_show();
                    break;
                
                case 4:
                    menu1_function(id);
                    break;

                default:
                    break;
            }
        }

    }
}

void calculatorstruct_init()
{
    calculatorstruct.last_keynum = BTN_ID_CALC_0;
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

    calculatorstruct_init();
}

/*菜单1功能*/
void menu1_function(btn_id_t id)
{
    switch (id)
    {
        case BTN_ID_MENU1_CALENDAR:
            lv_screen_load_anim(ui_calendar_ui,LV_SCREEN_LOAD_ANIM_OVER_TOP,10,0,false);
            break;

        case BTN_ID_MENU1_CALCULATOR:
            lv_screen_load_anim(ui_calculator,LV_SCREEN_LOAD_ANIM_OVER_TOP,10,0,false);
            break;

        case BTN_ID_MENU1_STOPWATCH:
            lv_screen_load_anim(ui_stopwatch_ui,LV_SCREEN_LOAD_ANIM_OVER_TOP,10,0,false);
            break;

        case BTN_ID_MENU1_CARDPACK:
            lv_screen_load_anim(ui_card_pack,LV_SCREEN_LOAD_ANIM_OVER_TOP,10,0,false);
            break;

        default:
            break;
    }
}

// BTN_ID_CALC_0 = 22,
//     BTN_ID_CALC_1 = 23,
//     BTN_ID_CALC_2 = 24,
//     BTN_ID_CALC_3 = 25,
//     BTN_ID_CALC_4 = 26,
//     BTN_ID_CALC_5 = 27,
//     BTN_ID_CALC_6 = 28,
//     BTN_ID_CALC_7 = 29,
//     BTN_ID_CALC_8 = 30,
//     BTN_ID_CALC_9 = 31,
//     BTN_ID_CALC_DOT = 32,
//     BTN_ID_CALC_ADD = 33,
//     BTN_ID_CALC_SUB = 34,
//     BTN_ID_CALC_MUL = 35,
//     BTN_ID_CALC_DIV = 36,
//     BTN_ID_CALC_EQ = 37,
//     BTN_ID_CALC_C = 38,
// double calculated_value,calculated_tempvalue1,calculated_tempvalue2;

/*当上一次为*/
void calculator_num(btn_id_t id)
{
    id -= 22;
    switch (calculatorstruct.Operator)
    {
        case BTN_ID_CALC_ADD:
            calculatorstruct.calculated_value += id;
            break;
        
        case BTN_ID_CALC_SUB:
            calculatorstruct.calculated_value -= id;
            break;

        case BTN_ID_CALC_MUL:
            calculatorstruct.calculated_value *= id;
            break;

        case BTN_ID_CALC_DIV:
            if(id)//被除数不能为0
            {
                calculatorstruct.calculated_value /= id;
            }
            break;

        default:
            break;
    }
}



/*计算器功能*/
void calculator_function(btn_id_t id)
{
    uint8_t last_keytype = calculator_typejudge(calculatorstruct.last_keynum);//判断上一次按键类型
    uint8_t now_keytype = calculator_typejudge(id);//判断当前按键类型

    switch (last_keytype)
    {
        case btn_type_calculator_num://判断上一个按键类型为数字
            uint8_t temp = id - BTN_ID_CALC_0;//得出当前值
            switch (now_keytype)
            {
                case btn_type_calculator_num://判断当前按键类型为数字
                    calculatorstruct.calculated_currentvalue  = calculatorstruct.calculated_currentvalue * 10 + temp;
                    break;
                
                case btn_type_calculator_Operator://判断当前按键类型为运算符
                    calculatorstruct.calculated_tempvalue = calculatorstruct.calculated_currentvalue;
                    break;

                default:
                    break;
            }
            break;
            
        case btn_type_calculator_Operator://上一个按键类型为运算符
            calculatorstruct.calculated_value += calculatorstruct.calculated_tempvalue + calculatorstruct.calculated_currentvalue;//计算所有值
            calculatorstruct.calculated_currentvalue = 0;
            calculatorstruct.calculated_tempvalue = 0;
            break;

        default:
            break;
    }

    last_keytype = now_keytype;//将当前按下的按键类型赋值给上一个按键类型
}