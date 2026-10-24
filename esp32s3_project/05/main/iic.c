#include "sys.h"

/*tft屏 iic通信引脚初始化*/
void tft_iic_gpio_init()
{
    gpio_config_t gpio_struct = {0};
    gpio_struct.pin_bit_mask = (1ULL << tft_iic_scl_bit) | (1ULL << tft_iic_sda_bit);
    gpio_struct.mode = GPIO_MODE_INPUT_OUTPUT_OD;//开漏
    gpio_struct.pull_up_en = GPIO_PULLUP_ENABLE;//上拉
    gpio_config(&gpio_struct);

    gpio_set_level(tft_iic_scl, 1);
    gpio_set_level(tft_iic_sda, 1);
}

// 起始信号：SCL为高电平的时候，SDA由高电平向低电平跳变。
void iic_start()
{
    gpio_set_level(tft_iic_scl, 0);
    esp_rom_delay_us(2);
    gpio_set_level(tft_iic_sda, 1);
    esp_rom_delay_us(2);
    gpio_set_level(tft_iic_scl, 1);
    while(!gpio_get_level(tft_iic_scl));//兼容拉伸
    esp_rom_delay_us(2);
    gpio_set_level(tft_iic_sda, 0);
    esp_rom_delay_us(2);
    gpio_set_level(tft_iic_scl, 0);//结束
    gpio_set_level(tft_iic_sda, 1);//释放sda
}

// 结束信号：SCL为高电平的时候，SDA由低电平向高电平跳变。
void iic_stop()
{
    gpio_set_level(tft_iic_scl, 0);
    esp_rom_delay_us(2);
    gpio_set_level(tft_iic_sda, 0);
    esp_rom_delay_us(2);
    gpio_set_level(tft_iic_scl, 1);
    while(!gpio_get_level(tft_iic_scl));//兼容拉伸
    esp_rom_delay_us(2);
    gpio_set_level(tft_iic_sda, 1);
    esp_rom_delay_us(2);
    gpio_set_level(tft_iic_scl, 0);//结束
    gpio_set_level(tft_iic_sda, 1);//释放sda
}

// I2C总线上的所有数据都是以8位字节传送的，发送器每发送一个字节，就在时钟脉冲9期间释放数据线，由接收器反馈一个应答信号。应答信号为低电平时，规定为有效应答位（ACK简称应答位），表示接收器已经成功地接收了该字节；应答信号为高电平时，规定为非应答位（NACK），一般表示接收器接收该字节没有成功，对于反馈有效应答位ACK的要求是，接收器在第9个时钟脉冲之前的低电平期间将SDA线拉低，并且确保在该时钟的高电平期间为稳定的低电平。如果接收器是主控器，则在它收到最后一个字节后，发送一个NACK信号，以通知被控发送器结束数据发送，并释放SDA线，以便主控接收器发送一个停止信号P
static void iic_ack()
{
    gpio_set_level(tft_iic_sda, 0);
    gpio_set_level(tft_iic_scl, 1);
    while(!gpio_get_level(tft_iic_scl));//兼容拉伸
    esp_rom_delay_us(2);
    gpio_set_level(tft_iic_scl, 0);//结束
    gpio_set_level(tft_iic_sda, 1);//释放sda
}

static void iic_nack()
{
    gpio_set_level(tft_iic_sda, 1);
    gpio_set_level(tft_iic_scl, 1);
    while(!gpio_get_level(tft_iic_scl));//兼容拉伸
    esp_rom_delay_us(2);
    gpio_set_level(tft_iic_scl, 0);//结束
    gpio_set_level(tft_iic_sda, 1);//释放sda
}

// 写时序：开始信号：主机+从设备地址+写命令，从机应答，应答成功，表示有这个设备，然后主机+设备内部寄存器地址，此时不用再加写命令控制字，从机应答，应答成功，表示设备内有这个地址，主机写入数据，从机应答，是否继续发送，不发送的话，发送停止信号P。
uint8_t iic_write_byte(uint8_t byte)
{
    /*写字节*/
    for (uint8_t i = 0; i < 8; i++)
    {
        
        if(byte & (0x80 >> i))
        {
            gpio_set_level(tft_iic_sda, 1);
        }
        else
        {
            gpio_set_level(tft_iic_sda, 0);
        }

        esp_rom_delay_us(2);
        gpio_set_level(tft_iic_scl, 1);
        while(!gpio_get_level(tft_iic_scl));//兼容拉伸
        esp_rom_delay_us(2);
        gpio_set_level(tft_iic_scl, 0);
        gpio_set_level(tft_iic_sda, 1);//释放sda
        esp_rom_delay_us(2);
    }

    /*收应答位*/
    gpio_set_level(tft_iic_scl, 1);
    while(!gpio_get_level(tft_iic_scl));//兼容拉伸
    if(gpio_get_level(tft_iic_sda))
    {
        gpio_set_level(tft_iic_scl, 0);
        return 1;
    }
    else
    {
        gpio_set_level(tft_iic_scl, 0);
        return 0;
    }
}

// 读时序：要想读设备，首先要知道将要所读取设备的地址告诉从设备，从设备才能将数据放到（发送）SDA上使主设备读取，从设备将数据放入SDA上的过程，由硬件主动完成，不用人为的写入。所以首先先写入从机地址，然后+写控制命令，从机应答，应答成功，表示有这个设备，然后写入内部寄存器地址，此时不用再加写命令控制字，从机应答，应答成功，表示设备内有这个地址。然后主机继续发出：写入从机地址，然后+读命令，从机应答，应答成功，此时便可以读取数据了，从设备已经将数据放入到SDA上了。地址跟设备已经验证了，不用再进行验证。

/*读1字节数据*/
/*传入参数为是否应答，1为应答，0为非应答*/
uint8_t iic_read_byte(uint8_t ack)
{
    /*读字节*/
    uint8_t data = 0;
    gpio_set_level(tft_iic_sda, 1);//释放sda
    gpio_set_level(tft_iic_scl, 0);
    esp_rom_delay_us(2);

    for (uint8_t i = 0; i < 8; i++)
    {
        gpio_set_level(tft_iic_scl, 1);
        while(!gpio_get_level(tft_iic_scl));//兼容拉伸
        esp_rom_delay_us(2);
        
        if(gpio_get_level(tft_iic_sda))
        {
            data = data | (0x80 >> i);
        }
        
        gpio_set_level(tft_iic_scl, 0);
        esp_rom_delay_us(2);
    }

    /*是否应答*/
    if(ack)
    {
        /*发应答位*/
        iic_ack();
    }
    else
    {
        /*发非应答位*/
        iic_nack();
    }

    return data;
}