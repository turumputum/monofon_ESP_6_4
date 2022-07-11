idf_component_register(SRCS "audioPlayer.c"
                    INCLUDE_DIRS "include"
                    REQUIRES audio_sal audio_pipeline audio_stream audio_hal fatfs stateConfig my_board esp_peripherals)
