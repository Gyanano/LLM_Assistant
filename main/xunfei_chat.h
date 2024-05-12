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

#define SYSTEM_SETTING  "Now you are a helpful assistant, Acy. You are good at answering people's questions.(Your response should be as concise as possible while ensuring accuracy)"

#define CHAT_MAX_HISTORY 10

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

/* The role of the message */
typedef enum {
    MSG_ROLE_SYSTEM = 0,
    MSG_ROLE_ASSISTANT,
    MSG_ROLE_USER,
    MSG_ROLE_UNKNOWN
} msg_role_t;

/* The chat message block */
typedef struct {
    char *role;     // The role of the message, "system", "assistant" or "user" 
    char *content;  // The content of the message
    msg_role_t role_type;
    int length;   // The length of the message
} chat_msg_t;

/* The chat message history */
typedef struct {
    chat_msg_t* msg[CHAT_MAX_HISTORY];
    int count;
} chat_history_t;


extern char xunfei_auth_url[352];
extern char chat_answer[2048];

extern void clear_chat_answer(void);
extern char *generate_json_params(const char* app_id, const char* model_name);
extern void update_chat_history(chat_msg_t *msg);
extern void free_temp_p(void);
extern void parse_chat_response(const char *data);
extern void update_auth_url(app_mode_t mode);