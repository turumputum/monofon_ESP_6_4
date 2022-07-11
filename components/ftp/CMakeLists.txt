idf_component_register(SRCS "ftp.c"
                    INCLUDE_DIRS "include"
                    REQUIRES stateConfig nvs_flash mdns tcpip_adapter)
