#include "sys.h"

/*最小为10ns*/
void ws2812_ns_delay(unsigned int ns)
{
    ns /= 10;//每句空循环耗时10ns
    while (ns--)
    {
        __asm__ volatile("nop");
    }
}

/*最小为10ms*/
void ws2812_ms_delay(unsigned long ms)
{
    ms *= 5700;//每句空循环耗时10ns
    while (ms--)
    {
        __asm__ volatile("nop");
    }
}

void ws2812_gpio_init()
{
    gpio_config_t gpio_struct;
    gpio_struct.pin_bit_mask = (1ULL << ws2812_i);
    gpio_struct.mode = GPIO_MODE_OUTPUT;
    gpio_config(&gpio_struct);

    set_ws2812(0);

}

//T1H 580ns~1us高 T1L 220ns~420ns低
void ws_2812_set1()
{
    set_ws2812(1);
    ws2812_ns_delay(180);
    set_ws2812(0);
    __asm__ volatile("nop"); __asm__ volatile("nop");
    __asm__ volatile("nop"); __asm__ volatile("nop");
    __asm__ volatile("nop"); __asm__ volatile("nop");
    __asm__ volatile("nop"); __asm__ volatile("nop");
    __asm__ volatile("nop"); __asm__ volatile("nop");
    __asm__ volatile("nop"); __asm__ volatile("nop");
}

//T0H 220ns~380ns高 T0L 580ns~1us低
void ws_2812_set0()
{
    set_ws2812(1);
    __asm__ volatile("nop"); __asm__ volatile("nop");
    __asm__ volatile("nop"); __asm__ volatile("nop");
    __asm__ volatile("nop"); __asm__ volatile("nop");
    __asm__ volatile("nop"); __asm__ volatile("nop");
    __asm__ volatile("nop"); __asm__ volatile("nop");
    __asm__ volatile("nop"); __asm__ volatile("nop");
    set_ws2812(0);
    ws2812_ns_delay(180);
   
}

void WS_2812_set(unsigned char r,unsigned char g,unsigned char b)
{
    set_ws2812(0);
    vTaskDelay(pdMS_TO_TICKS(1));
    for(unsigned char i = 0; i < 8; i++)
    {
        if(g & (0x80 >> i))
        {
            ws_2812_set1(); 
        }
        else
        {
            ws_2812_set0();
        }
    }
    for(unsigned char i = 0; i < 8; i++)
    {
        if(r & (0x80 >> i))
        {
            ws_2812_set1(); 
        }
        else
        {
            ws_2812_set0();
        }
    }
    for(unsigned char i = 0; i < 8; i++)
    {
        if(b & (0x80 >> i))
        {
            ws_2812_set1(); 
        }
        else
        {
            ws_2812_set0();
        }
    }
    set_ws2812(0);
    vTaskDelay(pdMS_TO_TICKS(1)); 
}