#include <stdio.h>

#include "ui_menu1.h"

#include "ui_btn.h"

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