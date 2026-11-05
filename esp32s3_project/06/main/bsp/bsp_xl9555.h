#ifndef     _BSP_XL9555_H_
#define     _BSP_XL9555_H_

#include <stdint.h>

typedef struct{
    uint8_t Input_Port_0,Input_Port_1,Output_Port_0,Output_Port_1,Polarity_Inversion_Port_0,Polarity_Inversion_Port_1,
    Configuration_Port_0,Configuration_Port_1;
}xl9555structtypedef;

extern xl9555structtypedef xl9555struct;

void xl9555struct_DeInit();//结构体赋初值
void xl9555_io_mode_init(uint8_t Configuration_Port, uint8_t num, uint8_t mode_value);//初始化引脚模式
void xl9555_writebit(uint8_t Output_Port, uint8_t num, uint8_t value);//输出高低电平

#endif
