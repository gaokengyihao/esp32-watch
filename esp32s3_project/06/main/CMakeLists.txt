idf_component_register(
    SRC_DIRS      "bsp" "driver" "app" "image"
    INCLUDE_DIRS  "." "bsp" "driver" "app"
    REQUIRES      esp_driver_gpio esp_driver_spi esp_timer lvgl nvs_flash
                  app_update esp_partition bt ble_uart)