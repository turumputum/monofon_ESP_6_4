set(COMPONENT_SRCS "msc_disk.c" "cmd.c" "spisd.c" "main.c" "usb_descriptors.c" "cdc.c" "freertos_hook.c")
set(COMPONENT_ADD_INCLUDEDIRS .)


#message("CMAKE_CURRENT_LIST_DIR is ${CMAKE_CURRENT_LIST_DIR}")

INCLUDE(${CMAKE_CURRENT_LIST_DIR}/../components/tusb/CMakeLists_flags.txt)

register_component()
