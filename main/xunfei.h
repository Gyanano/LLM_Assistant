#pragma once

#include <cJSON.h>
#include "util.h"

#define APP_ID      "0b22750b"
#define API_SECRET  "ZDBlMTUyZTRkMGMxNDgyNjZkZTI0MGM3"
#define API_KEY     "3317db78ddb6666ecd453dc2900cf423"

/* The LLM Application */
#define MODEL       "generalv3.5"
#define CHAT_URL    "ws://spark-api.xf-yun.com/v3.5/chat"
#define CHAT_HOST   "spark-api.xf-yun.com"
#define CHAT_PATH   "/v3.5/chat"

/* The STT(Speech To Text) Application */
#define STT_URL     "ws://iat-api.xfyun.cn/v2/iat"
#define STT_HOST    "iat-api.xfyun.cn"
#define STT_PATH    "/v2/iat"

/* The TTS(Text To Speech) Application */
#define TTS_URL     "ws://tts-api.xfyun.cn/v2/tts"
#define TTS_HOST    "tts-api.xfyun.cn"
#define TTS_PATH    "/v2/tts"

/* The Application mode: Chat, STT, TTS */
typedef enum {
    MODE_CHAT = 0,
    MODE_STT,
    MODE_TTS,
    MODE_UNKNOWN
} app_mode_t;


extern void update_auth_url(app_mode_t mode);