#include <stdio.h>
#include "main.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "util.h"

static const char *TAG = "MAIN";


static void Task_Print(void *args)
{
    ESP_LOGI(TAG, "Task_Print started");
    while (1)
    {
        ESP_LOGI(TAG, "Hello, world!");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
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
