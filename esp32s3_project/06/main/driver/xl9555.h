#ifndef     _XL9555_H_
#define     _XL9555_H_

#include <stdint.h>

extern xl9555structtypedef xl9555struct;

void xl9555struct_DeInit();//结构体赋初值，使用xl9555前必须调用
void xl9555_io_mode_init(uint8_t Configuration_Port, uint8_t num, uint8_t mode_value);//初始化引脚模式
void xl9555_writebit(uint8_t Output_Port, uint8_t num, uint8_t value);//输出高低电平

#endif
