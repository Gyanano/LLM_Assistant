#pragma once

#include "driver/gpio.h"

#define ESP_INTR_FLAG_DEFAULT 0
#define KEY_NUM         GPIO_NUM_9


extern void on_key_down_handler_register(void *handler);
extern void on_key_up_handler_register(void *handler);
extern void key_init(void);
extern void key_scan_task(void *args);
extern void install_key_isr(void);