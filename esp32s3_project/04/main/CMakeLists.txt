idf_component_register(SRCS "ws_2812.c" 
                            "main.c"
                            "gpio.c"
                            REQUIRES esp_driver_gpio
                    INCLUDE_DIRS ".")
