#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "ui.h"
#include "ui_btn.h"
#include "ui_calculator.h"

#define CALC_INPUT_DIGITS_MAX 9

typedef enum
{
    CALC_STATE_INPUT = 0,
    CALC_STATE_RESULT
} calc_state_t;

typedef struct
{
    char input[16];       /* 当前输入，例如 "12.5" */
    char expr[64];        /* 显示的整个算式，例如 "12+3=15" */

    double acc;           /* 左操作数/累计结果 */
    char op;              /* 当前待计算运算符：+ - * / */

    calc_state_t state;
    uint8_t has_dot;
    uint8_t error;
} calculator_t;

static calculator_t calc;

static uint8_t calc_input_digits(void)
{
    uint8_t n = 0;
    const char *p = calc.input;

    while(*p)
    {
        if(*p >= '0' && *p <= '9')
        {
            n++;
        }
        p++;
    }

    return n;
}

static void calc_append_expr(const char *s)
{
    size_t len = strlen(calc.expr);
    size_t add = strlen(s);

    if(len + add + 1 < sizeof(calc.expr))
    {
        strcat(calc.expr, s);
    }
}

static void calc_append_expr_char(char c)
{
    char s[2];
    s[0] = c;
    s[1] = '\0';
    calc_append_expr(s);
}

static void calc_clear_state(void)
{
    memset(&calc, 0, sizeof(calc));
    calc.state = CALC_STATE_INPUT;
    calc.has_dot = 0;
    calc.error = 0;
}

static void calc_render(void)
{
    if(calc.error)
    {
        lv_label_set_text(ui_calc_expression, "Error");
        lv_label_set_text(ui_calc_value, "Error");
        return;
    }

    lv_label_set_text(ui_calc_expression, calc.expr);
    lv_label_set_text(ui_calc_value, calc.input[0] ? calc.input : "0");
}

static double calc_get_input(void)
{
    return strtod(calc.input, NULL);
}

static uint8_t calc_apply(double a, char op, double b, double *out)
{
    switch(op)
    {
        case '+': *out = a + b; break;
        case '-': *out = a - b; break;
        case '*': *out = a * b; break;
        case '/':
            if(b == 0.0)
            {
                calc.error = 1;
                return 0;
            }
            *out = a / b;
            break;

        default:
            *out = b;
            break;
    }

    return 1;
}

static void calc_digit(uint8_t d)
{
    if(calc.error)
    {
        return;
    }

    if(calc.state == CALC_STATE_RESULT)
    {
        calc_clear_state();
    }

    if(calc_input_digits() >= CALC_INPUT_DIGITS_MAX)
    {
        return;
    }

    size_t len = strlen(calc.input);

    if(len + 1 >= sizeof(calc.input))
    {
        return;
    }

    calc.input[len] = (char)('0' + d);
    calc.input[len + 1] = '\0';

    calc_append_expr_char((char)('0' + d));
    calc_render();
}

static void calc_dot(void)
{
    if(calc.error)
    {
        return;
    }

    if(calc.state == CALC_STATE_RESULT)
    {
        calc_clear_state();
    }

    if(calc.has_dot)
    {
        return;
    }

    if(calc_input_digits() >= CALC_INPUT_DIGITS_MAX)
    {
        return;
    }

    if(calc.input[0] == '\0')
    {
        strcpy(calc.input, "0");
        calc_append_expr_char('0');
    }

    size_t len = strlen(calc.input);

    if(len + 1 >= sizeof(calc.input))
    {
        return;
    }

    calc.input[len] = '.';
    calc.input[len + 1] = '\0';
    calc.has_dot = 1;

    calc_append_expr_char('.');
    calc_render();
}

static void calc_operator(char op)
{
    if(calc.error)
    {
        return;
    }

    /* 等号后直接按运算符：用当前结果继续计算 */
    if(calc.state == CALC_STATE_RESULT)
    {
        calc.acc = calc_get_input();
        strcpy(calc.expr, calc.input);
        calc.state = CALC_STATE_INPUT;
    }

    /* 连续按运算符：只替换运算符，不重复追加 */
    if(calc.input[0] == '\0' && calc.op != 0)
    {
        calc.op = op;

        size_t len = strlen(calc.expr);
        if(len > 0)
        {
            calc.expr[len - 1] = op;
        }

        calc_render();
        return;
    }

    /* 还没有输入数字，也没有前一个运算符 */
    if(calc.input[0] == '\0' && calc.op == 0)
    {
        return;
    }

    double value = calc_get_input();

    if(calc.op != 0)
    {
        double result;
        if(!calc_apply(calc.acc, calc.op, value, &result))
        {
            calc_render();
            return;
        }
        calc.acc = result;
    }
    else
    {
        calc.acc = value;
    }

    calc.op = op;
    calc_append_expr_char(op);

    calc.input[0] = '\0';
    calc.has_dot = 0;
    calc.state = CALC_STATE_INPUT;

    calc_render();
}

static void calc_equal(void)
{
    if(calc.error || calc.state == CALC_STATE_RESULT)
    {
        return;
    }

    if(calc.input[0] == '\0' && calc.op == 0)
    {
        return;
    }

    double value = calc.input[0] ? calc_get_input() : calc.acc;

    if(calc.op != 0)
    {
        double result;
        if(!calc_apply(calc.acc, calc.op, value, &result))
        {
            calc_render();
            return;
        }
        calc.acc = result;
    }
    else
    {
        calc.acc = value;
    }

    calc_append_expr_char('=');

    char result_buf[32];
    snprintf(result_buf, sizeof(result_buf), "%.9g", calc.acc);
    calc_append_expr(result_buf);

    snprintf(calc.input, sizeof(calc.input), "%.9g", calc.acc);
    calc.has_dot = (strchr(calc.input, '.') != NULL);
    calc.op = 0;
    calc.state = CALC_STATE_RESULT;
    calc_render();
}

static void calc_clear(void)
{
    calc_clear_state();
    calc_render();
}

void calculator_init(void)
{
    calc_clear_state();
    calc_render();
}

void calculator_key(btn_id_t id)
{
    if(id >= BTN_ID_CALC_0 && id <= BTN_ID_CALC_9)
    {
        calc_digit((uint8_t)(id - BTN_ID_CALC_0));
    }
    else if(id == BTN_ID_CALC_DOT)
    {
        calc_dot();
    }
    else if(id == BTN_ID_CALC_ADD)
    {
        calc_operator('+');
    }
    else if(id == BTN_ID_CALC_SUB)
    {
        calc_operator('-');
    }
    else if(id == BTN_ID_CALC_MUL)
    {
        calc_operator('*');
    }
    else if(id == BTN_ID_CALC_DIV)
    {
        calc_operator('/');
    }
    else if(id == BTN_ID_CALC_EQ)
    {
        calc_equal();
    }
    else if(id == BTN_ID_CALC_C)
    {
        calc_clear();
    }
}
