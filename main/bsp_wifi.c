#include "bsp_wifi.h"
// #include "esp_system.h"
// #include "esp_netif_sntp.h"
// #include "esp_event.h"
// #include "esp_log.h"

static const char *TAG = "WIFI_MODULE";

esp_periph_handle_t wifi_init(void)
{
    periph_wifi_cfg_t wifi_cfg = {
        .wifi_config.sta.ssid = WIFI_SSID,
        .wifi_config.sta.password = WIFI_PASSWORD,
    };
    esp_periph_handle_t wifi_handle = periph_wifi_init(&wifi_cfg);
    return wifi_handle;
}

// void wifi_sync_time(void)
// {
//     esp_netif_sntp_deinit(); // Deinitialize SNTP service
//     esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("ntp.aliyun.com");
//     esp_netif_sntp_init(&config); // Initialize SNTP service
//     if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK)
//     {
//         // If 10s timeout and the s_ntp_retry is less than WIFI_MAXIMUM_RETRY_NTP, then retry
//         if (s_ntp_retry < WIFI_MAXIMUM_RETRY_NTP)
//         {
//             s_ntp_retry++;
//             ESP_LOGW(TAG, "Failed to update system time within 10s timeout, retrying(%d/%d)...", s_ntp_retry, WIFI_MAXIMUM_RETRY_NTP);
//             wifi_sync_time();
//         }
//         else
//         {
//             // If 10s timeout and the s_ntp_retry is greater than WIFI_MAXIMUM_RETRY_NTP, then abort
//             ESP_LOGE(TAG, "Retry times exceeded, aborting...");
//             abort();
//         }
//     }
//     else
//     {
//         s_ntp_retry = 0;
//     }
// }