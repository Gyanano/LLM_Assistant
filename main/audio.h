#pragma once

#include "esp_err.h"

#define AUDIO_VOICE_VOLUME    CONFIG_VOICE_VOLUME

extern esp_err_t audio_init(void);