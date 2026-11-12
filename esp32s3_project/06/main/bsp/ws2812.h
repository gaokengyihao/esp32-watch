#ifndef     _WS2812_H_
#define     _WS2812_H_

/*ws2812*/
#define     ws2812              GPIO_NUM_12
#define     ws2812_writebit(x)  gpio_set_level(ws2812, x)

void ws_2812_set1();
void ws_2812_set0();
void ws2812_gpio_init();
void ws2812_ns_delay(unsigned int ns);
void ws2812_ms_delay(unsigned long ms);
void WS_2812_set(unsigned char r,unsigned char g,unsigned char b);

#endif
