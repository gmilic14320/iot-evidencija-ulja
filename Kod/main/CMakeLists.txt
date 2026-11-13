idf_component_register(SRCS "main.c" "prikaz_kolicine.c"
                       INCLUDE_DIRS "."
                       REQUIRES hcsr04 lcd1602 tank podesavanje_tanka freertos log)
