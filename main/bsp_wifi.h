#pragma once

#include "esp_wifi.h"
#include "esp_peripherals.h"
#include "periph_wifi.h"

#define WIFI_SSID       CONFIG_ESP_WIFI_SSID
#define WIFI_PASSWORD   CONFIG_ESP_WIFI_PASSWORD

extern esp_periph_handle_t wifi_init(void);
// extern void wifi_sync_time(void);