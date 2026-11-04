idf_component_register(SRCS "xl9555.c" "display.c" "ota.c" "bluetooth.c" "image.c" "tft_touch.c" "tft_show.c" "iic.c" "image/image_Battery_level.c" "image/image_1.c" "lvgl_port.c" "spi.c" "task.c" "ws_2812.c" "main.c" "gpio.c"
                            REQUIRES esp_driver_gpio esp_driver_spi esp_timer lvgl bt ble_uart
                            ble_uart nvs_flash 
                            app_update esp_partition esp_lcd esp_lvgl_port
                    INCLUDE_DIRS ".")
