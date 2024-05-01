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

esp_websocket_client_handle_t g_client = NULL;


static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);


static void Task_Demo(void *args)
{
    int flag = 0;
    ESP_LOGI(TAG, "Task_Demo started");
    update_auth_url(MODE_CHAT);
    chat_msg_t msg = {
        .role_type = MSG_ROLE_USER,
        .role = "user",
        .content = "Hello, who are you?",
        .length = 19
    };
    update_chat_history(&msg);
    while (1)
    {
        if (flag == 0)
        {
            char* json_str = generate_json_params(APP_ID, MODEL);
            // ESP_LOGI(TAG, "The json params is: %s", json_str);

            // create a WebSocket client
            // ESP_LOGI(TAG, "auth url: %s", xunfei_auth_url);
            ws_init_by_uri(&g_client, xunfei_auth_url);
            // register the event handler
            ws_register_event_handler(&g_client, WEBSOCKET_EVENT_ANY, websocket_event_handler);
            // start the WebSocket client
            ws_start(&g_client);
            esp_websocket_client_send_text(g_client, json_str, strlen(json_str), portMAX_DELAY);
            esp_websocket_client_close(g_client, portMAX_DELAY);
            ws_destroy_client(&g_client);
            free(json_str);
            json_str = NULL;
            // free_temp_p();

            printf("The final answer: %s\n", chat_answer);
            flag = 1;
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

void enbale_GPIO11(void)
{
    printf("ESP_EFUSE_VDD_SPI_AS_GPIO start\n-----------------------------\n");
    esp_efuse_write_field_bit(ESP_EFUSE_VDD_SPI_AS_GPIO);

}

void app_main(void)
{
    enbale_GPIO11();
    
    ESP_ERROR_CHECK(nvs_init());

    wifi_init_sta();

    wifi_sync_time();

    char time_str[32];
    get_gmttime(time_str, sizeof(time_str)/sizeof(time_str[0]));
    ESP_LOGI(TAG, "Current GMT time: %s", time_str);

    enable_speaker();
    mic_err_t err = mic_init();
    if (err != MIC_OK)
    {
        ESP_LOGE(TAG, "Mic init failed: %d", err);
        return;
    }
    ESP_LOGI(TAG, "Mic init success");

    // The size of stack for this task must be greater than 2048 bytes, 
    // or the program will crash and reboot all the time.
    xTaskCreate(Task_Demo, "Task_Demo", 2048, NULL, 5, NULL);

    /* Echo the sound from MIC in echo mode */
    xTaskCreate(i2s_echo, "i2s_echo", 8192, NULL, 5, NULL);
}

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id)
    {
    case WEBSOCKET_EVENT_CONNECTED:  // 1
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
        clear_chat_answer();
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        break;
    case WEBSOCKET_EVENT_DATA: // 3
        // ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
        /*
            // opcode == 0x08 is a close message
            // if (data->op_code == 0x08 && data->data_len == 2) {
            //     ESP_LOGW(TAG, "Received closed message with code=%d", 256 * data->data_ptr[0] + data->data_ptr[1]);
            // } else {
            //     ESP_LOGW(TAG, "Received=%.*s\n\n", data->data_len, (char *)data->data_ptr);
            // }

            // // print the received data
            // ESP_LOGI(TAG, "Received data: %s", data->data_ptr);
            // The real data is in data->data_ptr
        */
        parse_chat_response((const char *)data->data_ptr);
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
        break;
    }
}