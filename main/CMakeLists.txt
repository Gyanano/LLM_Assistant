file(GLOB_RECURSE DEMO_SOURCES *.c)

# set(COMPONENT_SRCS "main.c" "baidu_stt.c" "baidu_tts.c" "minimax_chat.c" "lv_gui.c" "img_bilibili120.c" "daytime.c" "google_loop.c" "night.c" "font_alipuhui20.c")
set(COMPONENT_SRCS "main.c" "baidu_stt.c" "baidu_tts.c" "minimax_chat.c" "lv_gui.c" "img_bilibili120.c" "font_alipuhui20.c")
set(COMPONENT_ADD_INCLUDEDIRS .)

register_component()

# Some of warinings from LVGL. Block them.
target_compile_options(${COMPONENT_LIB} PRIVATE -w)
