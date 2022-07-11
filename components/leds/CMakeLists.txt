file(GLOB ws2812_rmt_drv_lib ws2812_rmt_drv/*.c)

idf_component_register(SRCS "leds.c" ${ws2812_rmt_drv_lib}
                    INCLUDE_DIRS "include" "ws2812_rmt_drv"
                    REQUIRES stateConfig)
