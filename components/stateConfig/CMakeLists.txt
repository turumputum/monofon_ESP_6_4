file(GLOB ini_lib ini/*.c)

idf_component_register(SRCS "stateConfig.c" ${ini_lib}
                    INCLUDE_DIRS "include" "ini"
                    REQUIRES audio_sal leds fatfs spiffs playlist log)
