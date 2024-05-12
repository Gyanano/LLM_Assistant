file(GLOB_RECURSE DEMO_SOURCES *.c)

set(COMPONENT_SRCS "baidu_stt.c" "audio.c" "minimax_chat.c" "baidu_tts.c" "main.c")
set(COMPONENT_ADD_INCLUDEDIRS .)

register_component()

# Some of warinings from LVGL. Block them.
target_compile_options(${COMPONENT_LIB} PRIVATE -w)
