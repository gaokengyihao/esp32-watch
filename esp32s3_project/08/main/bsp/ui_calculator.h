#ifndef     _UI_CALCULATOR_H_
#define     _UI_CALCULATOR_H_

#include <stdint.h>
#include "ui_btn.h"

/*计算器按键类型细分*/
typedef enum{
    btn_type_calculator_num = 1,
    btn_type_calculator_dot = 2,
    btn_type_calculator_operator = 3,
    btn_type_calculator_clear = 4,
    btn_type_calculator_equal = 5
}btn_type_calculator_t;

uint8_t calculator_typejudge(uint8_t id);
void calculator_function(btn_type_calculator_t btn_type, btn_id_t id);

#endif