idf_component_register(SRCS "lvgl_port.c" "spi.c" "tft.c" "task.c" "ws_2812.c" 
                            "main.c"
                            "gpio.c"
                            REQUIRES esp_driver_gpio esp_timer lvgl
                    INCLUDE_DIRS ".")
