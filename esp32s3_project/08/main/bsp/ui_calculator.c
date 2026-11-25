#include <stdio.h>
#include <string.h>

#include "ui.h"
#include "ui_btn.h"

#include "ui_calculator.h"



char buf[32], currenbuf[64], tempbuf[32], expressionbuf[32], operatorbuf[8];

typedef struct 
{
    double value, currentvalue, tempvalue, currentvalue_int, currentvalue_dec;
    uint8_t dot_flag, operator, char_operator, char_equal;
    uint32_t dec_scale;
}calculatorstructtypedef;

calculatorstructtypedef calculatorstruct;

/*按键类型判断*/
uint8_t calculator_typejudge(uint8_t id)
{
    uint8_t btn_type = 0;

    if(id >= BTN_ID_CALC_0 && id <= BTN_ID_CALC_9){btn_type = btn_type_calculator_num;}
    if(id == BTN_ID_CALC_DOT){btn_type = btn_type_calculator_dot;}
    if(id >= BTN_ID_CALC_ADD && id <= BTN_ID_CALC_DIV){btn_type = btn_type_calculator_operator;}
    if(id == BTN_ID_CALC_C){btn_type = btn_type_calculator_clear;}
    if(id == BTN_ID_CALC_EQ){btn_type = btn_type_calculator_equal;}

    return btn_type;
}

/*计算小数倍率*/
void calculator_dot()
{
    if (calculatorstruct.dot_flag)//第一次按下小数点
    {
        calculatorstruct.dec_scale *= 10;
    }
}

/*计算器 将按下按键转换成数据*/
void calculator_convert(uint8_t id)
{
    double temp = id - BTN_ID_CALC_0;
    switch (calculatorstruct.dot_flag)
    {
        case 0://小数点前
            calculatorstruct.currentvalue_int = calculatorstruct.currentvalue_int * 10 + temp;
            break;

        case 1://小数点后
            calculator_dot();
            calculatorstruct.currentvalue_dec = calculatorstruct.currentvalue_dec + temp / calculatorstruct.dec_scale;
            break;
        
        default:
            break;
    }
    calculatorstruct.currentvalue = calculatorstruct.currentvalue_int + calculatorstruct.currentvalue_dec;
}

/*计算*/
void calculator_result()
{
    switch (calculatorstruct.operator)
    {
        case BTN_ID_CALC_ADD:
            calculatorstruct.value += calculatorstruct.currentvalue;
            break;

        case BTN_ID_CALC_SUB:
            calculatorstruct.value -= calculatorstruct.currentvalue;
            break;

        case BTN_ID_CALC_MUL:
            calculatorstruct.value *= calculatorstruct.currentvalue;
            break;

        case BTN_ID_CALC_DIV:
            if(calculatorstruct.currentvalue != 0)
            {
                calculatorstruct.value /= calculatorstruct.currentvalue;
            }
            break;
        
        default:
            break;
    }
    calculatorstruct.currentvalue_int = 0;
    calculatorstruct.currentvalue_dec = 0;
    calculatorstruct.dec_scale = 0;
    calculatorstruct.dot_flag = 0;
}

/*计算器 按下运算符后*/
void calculator_operator(btn_id_t id)
{
    if(calculatorstruct.operator)//已经有运算符的情况下再按下运算符
    {
        calculator_result();
    }
    else//第一次按下运算符
    {
        calculatorstruct.value = calculatorstruct.currentvalue;
        calculatorstruct.currentvalue = 0;
        calculatorstruct.currentvalue_int = 0;
        calculatorstruct.currentvalue_dec = 0;
    }
    calculatorstruct.operator = id;//替换为新运算符
}

/*在显示时将运算符转成字符串*/
char calculator_operator_char(uint8_t op)
{
    switch(op)
    {
        case BTN_ID_CALC_ADD: return '+';
        case BTN_ID_CALC_SUB: return '-';
        case BTN_ID_CALC_MUL: return '*';
        case BTN_ID_CALC_DIV: return '/';
        default: return 0;
    }
}

/*计算器 按下等号后*/
void calculator_equal()
{
    calculatorstruct.tempvalue = calculatorstruct.currentvalue;//记录上一次运算最终结果，用于后面显示部分
    if(calculatorstruct.operator)//已经有运算符的情况下再按下运算符
    {
        calculator_result();
        calculatorstruct.char_equal = '=';
    }
}

// calculated_value, calculated_currentvalue, calculated_tempvalue
/*计算器显示*/
void calculator_show(btn_type_calculator_t btn_type)
{
    if(btn_type != btn_type_calculator_equal)//非等号
    {
        snprintf(currenbuf, sizeof(currenbuf), "%.6g", calculatorstruct.currentvalue);
        snprintf(buf, sizeof(buf), "%.6g", calculatorstruct.value);
        
        lv_label_set_text_fmt(ui_calc_expression, "%s%c%c", buf, calculatorstruct.char_operator, calculatorstruct.char_equal);

        lv_label_set_text(ui_calc_value, currenbuf);
    }
    else//等号
    {
        snprintf(tempbuf, sizeof(tempbuf), "%.6g", calculatorstruct.tempvalue);
        snprintf(currenbuf, sizeof(currenbuf), "%.6g", calculatorstruct.currentvalue);
        snprintf(buf, sizeof(buf), "%.6g", calculatorstruct.value);
        
        lv_label_set_text_fmt(ui_calc_expression, "%s%c%s%c", tempbuf, calculatorstruct.char_operator,currenbuf, calculatorstruct.char_equal);
        
        lv_label_set_text(ui_calc_value, buf);//下面显示运算结果

        calculatorstruct.char_equal = 0;//等号只在按下后显示一次，然后将等号清零
    }
    
}

/*计算器 总功能*/
void calculator_function(btn_type_calculator_t btn_type, btn_id_t id)
{
    switch (btn_type)
    {
        case btn_type_calculator_num://数字
            calculator_convert(id);
            break;
        
        case btn_type_calculator_dot://小数点
            if(!calculatorstruct.dot_flag){calculatorstruct.dec_scale = 1;}//第一次按下小数点，给小数倍率赋初值
            calculatorstruct.dot_flag = 1;
            break;

        case btn_type_calculator_operator://运算符
            calculator_operator(id);
            calculatorstruct.char_operator = calculator_operator_char(id);
            break;

        case btn_type_calculator_clear://清零
            memset(&calculatorstruct, 0, sizeof(calculatorstruct));
            break;
        
        case btn_type_calculator_equal://等于
            calculator_equal();
            break;

        default:
            break;
    }
    calculator_show(btn_type);
}