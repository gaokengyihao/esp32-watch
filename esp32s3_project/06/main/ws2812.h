#ifndef     _WS2812_H_
#define     _WS2812_H_

#define     ws2812_r    1
#define     ws2812_g    2
#define     ws2812_b    3

#define     ws2812_i         48
#define     set_ws2812(x)    gpio_set_level(GPIO_NUM_48, x)

void ws_2812_set1();
void ws_2812_set0();
void ws2812_gpio_init();
void ws2812_ns_delay(unsigned int ns);
void ws2812_ms_delay(unsigned long ms);
void WS_2812_set(unsigned char r,unsigned char g,unsigned char b);

#endif
