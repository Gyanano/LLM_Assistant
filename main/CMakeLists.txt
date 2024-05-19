file(GLOB_RECURSE DEMO_SOURCES *.c)

# set(COMPONENT_SRCS "font_led.c" "deepseek_chat.c" "_clock_alpha_60x60.c" "_setting_alpha_80x80.c" "_message_alpha_80x80.c" "_weather_alpha_80x80.c" "main.c" "baidu_stt.c" "baidu_tts.c" "minimax_chat.c" "lv_gui.c" "img_bilibili120.c" "daytime.c" "google_loop.c" "night.c" "font_alipuhui20.c")
set(COMPONENT_SRCS "main.c" "baidu_stt.c" "baidu_tts.c" "minimax_chat.c" "deepseek_chat.c" "lv_gui.c" "img_bilibili120.c" "font_led.c" "_message_alpha_80x80.c" "_weather_alpha_80x80.c" "_setting_alpha_80x80.c" "_clock_alpha_60x60.c" "_back_black_alpha_16x16.c" "font_alipuhui20.c")
set(COMPONENT_ADD_INCLUDEDIRS .)

register_component()

# Some of warinings from LVGL. Block them.
target_compile_options(${COMPONENT_LIB} PRIVATE -w)
