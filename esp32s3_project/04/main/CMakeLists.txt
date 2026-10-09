idf_component_register(SRCS "spi.c" "tft.c" "task.c" "ws_2812.c" 
                            "main.c"
                            "gpio.c"
                            REQUIRES esp_driver_gpio
                    INCLUDE_DIRS ".")
