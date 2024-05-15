#ifndef _DEEPSEEK_CHAT_H_
#define _DEEPSEEK_CHAT_H_

#define DEEPSEEK_KEY    "sk-82acc135ebc34f638a4b0ffa43a1e3fb"


#ifdef __cplusplus
extern "C" {
#endif

extern char deepseek_content[2048];

char *deepseek_chat(const char *text);

#ifdef __cplusplus
}
#endif

#endif