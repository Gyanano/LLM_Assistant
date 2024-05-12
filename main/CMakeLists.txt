file(GLOB_RECURSE DEMO_SOURCES *.c)

set(COMPONENT_SRCS "bsp_wifi.c" "baidu_stt.c" "bsp_key.c" "audio.c" "xunfei_chat.c" "ws_client.c" "util.c" "main.c")
set(COMPONENT_ADD_INCLUDEDIRS .)

register_component()

# Some of warinings from LVGL. Block them.
target_compile_options(${COMPONENT_LIB} PRIVATE -w)
