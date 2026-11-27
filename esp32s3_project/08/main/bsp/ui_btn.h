#ifndef     _UI_BTN_H_
#define     _UI_BTN_H_

typedef enum{
    /*菜单1*/
    btn_type_menu1 = 1,
    /*计算器*/
    btn_type_calculator = 2,
    /*秒表*/
    btn_type_stopwatch = 3
}btn_type_t;//以模块区分按键

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

#endif