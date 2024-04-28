#include "ws_client.h"
#include "esp_log.h"

static const char *TAG = "WS_CLIENT";

const esp_websocket_client_config_t ws_cfg = {
    .uri = "ws://124.222.224.186",
    .port = 8800, // Only used in development environment
};
esp_websocket_client_handle_t client = NULL;

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    // esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    ESP_LOGI(TAG, "event_id=%d", (int)event_id);
    switch (event_id)
    {
    case WEBSOCKET_EVENT_CONNECTED:  // 1
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        break;
    case WEBSOCKET_EVENT_DATA: // 3
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
        break;
    }
}

void create_ws_client(void)
{
    client = esp_websocket_client_init(&ws_cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);
    esp_websocket_client_start(client);
    ESP_LOGI(TAG, "Websocket client started");

    char data[32];
    int i = 0;
    while (i < 5)
    {
        if (esp_websocket_client_is_connected(client))
        {
            int len = sprintf(data, "hello %04d", i++);
            ESP_LOGI(TAG, "Sending %s", data);
            esp_websocket_client_send_text(client, data, len, portMAX_DELAY);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    esp_websocket_client_close(client, portMAX_DELAY);
}

void destroy_ws_client(esp_websocket_client_handle_t client)
{
    esp_websocket_client_destroy(client);
    ESP_LOGI(TAG, "Websocket client destroyed");
}

void ws_test_app(void)
{
    create_ws_client();
    destroy_ws_client(client);
}