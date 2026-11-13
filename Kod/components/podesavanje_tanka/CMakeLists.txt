idf_component_register(SRCS "podesavanje_tanka.c"
                       INCLUDE_DIRS "include"
                       REQUIRES tank nvs_flash espressif__cjson esp_driver_uart)
