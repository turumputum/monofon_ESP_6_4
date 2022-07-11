file(GLOB decode_jpeg_lib decode_jpeg/*.c)
file(GLOB fontx_lib fontx/*.c)

idf_component_register(SRCS "st7789.c" ${decode_jpeg_lib} ${fontx_lib}
                    INCLUDE_DIRS "include" "decode_jpeg" "fontx"
                    REQUIRES fatfs log)
