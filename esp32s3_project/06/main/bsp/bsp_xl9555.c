#include "board.h"

typedef struct{
    uint8_t Input_Port_0,Input_Port_1,Output_Port_0,Output_Port_1,Polarity_Inversion_Port_0,Polarity_Inversion_Port_1,
    Configuration_Port_0,Configuration_Port_1;
}xl9555structtypedef;

xl9555structtypedef xl9555struct;

/*地址*/
// Bit 7 6 5 4 3 2 1 0
// Symbol 0 1 0 0 A2 A1 A0 R/W
// 开发板上A2 A1 A0均接地为0
// 写发0，读发1
#define xl955_address_write 0x40
#define xl955_address_read  0x41

/*发完地址发命令，相当于指针，指向寄存器。发完后，不重新发命令则会继续对此寄存器操作*/
#define Input_Port_0_command                0x00
#define Input_Port_1_command                0x01
#define Output_Port_0_command               0x02
#define Output_Port_1_command               0x03
#define Polarity_Inversion_Port_0_command   0x04
#define Polarity_Inversion_Port_1_command   0x05
#define Configuration_Port_0_command        0x06
#define Configuration_Port_1_command        0x07

void xl9555struct_DeInit()
{
    xl9555struct.Configuration_Port_0 = 0xff;
    xl9555struct.Configuration_Port_1 = 0xff;
    xl9555struct.Output_Port_0 = 0xff;
    xl9555struct.Output_Port_1 = 0xff;
    xl9555struct.Polarity_Inversion_Port_0 = 0;
    xl9555struct.Polarity_Inversion_Port_1 = 0;
}

#define configuration_Port        0b11110011//1.3 1.2设为输出 //1.3 slcd_pwr 1.2 slcd_rst
/***********************************************
函数名称：
函数功能：设置引脚模式
入口参数：Configuration_Port：寄存器0或1
         num：0~7中第几位
         mode_value：设置输出输入状态，0为输出，1为输入
出口参数：
备注：注意：设置配置时除输出外全置1（高阻），不配置则默认为上拉输入，置1为高阻
************************************************/
void xl9555_io_mode_init(uint8_t Configuration_Port, uint8_t num, uint8_t mode_value)
{
    iic_start();
    iic_write_byte(xl955_address_write);

    if(Configuration_Port == 1)
    {
        if(mode_value)
        {
            xl9555struct.Configuration_Port_1 |= (0x01 << num);
        }
        else
        {
            xl9555struct.Configuration_Port_1 &= ~(0x01 << num);
        }
        iic_write_byte(Configuration_Port_1_command);
        iic_write_byte(xl9555struct.Configuration_Port_1);
    }
    else if(Configuration_Port == 0)
    {
        if(mode_value)
        {
            xl9555struct.Configuration_Port_0 |= (0x01 << num);
        }
        else
        {
            xl9555struct.Configuration_Port_0 &= ~(0x01 << num);
        }
        iic_write_byte(Configuration_Port_0_command);
        iic_write_byte(xl9555struct.Configuration_Port_0);
    }

    iic_stop();
}

/*设置io1引脚输出电平*/
void xl9555_writebit(uint8_t Output_Port, uint8_t num, uint8_t value)
{
    iic_start();
    iic_write_byte(xl955_address_write);//地址+写

    if(Output_Port == 1)
    {
        if(value)
        {
            xl9555struct.Output_Port_1 |= (0x01 << num);
        }
        else
        {
            xl9555struct.Output_Port_1 &= ~(0x01 << num);
        }
        iic_write_byte(Output_Port_1_command);
        iic_write_byte(xl9555struct.Output_Port_1);
    }
    else if(Output_Port == 0)
    {
        if(value)
        {
            xl9555struct.Output_Port_0 |= (0x01 << num);
        }
        else
        {
            xl9555struct.Output_Port_0 &= ~(0x01 << num);
        }
        iic_write_byte(Output_Port_0_command);
        iic_write_byte(xl9555struct.Output_Port_0);
    }

    iic_stop();
}