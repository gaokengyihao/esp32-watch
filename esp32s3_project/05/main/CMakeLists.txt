idf_component_register(SRCS "tft_touch.c" "tft_show.c" "iic.c" "image/image_Battery_level.c" "image/image_1.c" "lvgl_port.c" "spi.c" "task.c" "ws_2812.c" 
                            "main.c"
                            "gpio.c"
                            REQUIRES esp_driver_gpio esp_timer lvgl
                    INCLUDE_DIRS ".")
