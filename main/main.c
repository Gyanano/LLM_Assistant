#include <stdio.h>
#include "main.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_module.h"
#include "ws_client.h"
#include "util.h"
#include "xunfei.h"

static const char *TAG = "MAIN";


static void Task_Print(void *args)
{
    int i = 0;
    ESP_LOGI(TAG, "Task_Print started");
    update_auth_url(MODE_CHAT);
    while (1)
    {
        if (i < 10)
        {
            ESP_LOGI(TAG, "Hello, world!");
            i++;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

static esp_err_t nvs_init(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_init());

    wifi_init_sta();

    wifi_sync_time();

    ws_test_app();

    char time_str[32];
    get_gmttime(time_str, sizeof(time_str)/sizeof(time_str[0]));
    ESP_LOGI(TAG, "Current GMT time: %s", time_str);

    mic_err_t err = mic_init();
    if (err != MIC_OK)
    {
        ESP_LOGE(TAG, "Mic init failed: %d", err);
        return;
    }
    ESP_LOGI(TAG, "Mic init success");

    // The size of stack for this task must be greater than 2048 bytes, 
    // or the program will crash and reboot all the time.
    xTaskCreate(Task_Print, "Task_Print", 2048, NULL, 5, NULL);
}
