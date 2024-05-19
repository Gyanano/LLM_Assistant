#pragma once

#include "time.h"
#include "driver/ledc.h"

#define USE_MINIMAX         1
#define USE_DEEPSEEK        0

// W (28461) AUDIO_THREAD: Make sure selected the `CONFIG_SPIRAM_BOOT_INIT` and `CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY` by `make menuconfig`

extern float bg_duty;
extern time_t now;
extern struct tm timeinfo;
extern char g_temp[10];
extern char g_humidity[10];
