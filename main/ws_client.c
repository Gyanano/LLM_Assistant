#include "ws_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

static const char *TAG = "WS_CLIENT";

// const esp_websocket_client_config_t ws_cfg = {
//     .uri = "ws://124.222.224.186",
//     .port = 8800, // Only used in development environment
// };
esp_websocket_client_config_t ws_cfg = {};
esp_websocket_client_handle_t g_client = NULL;

static TimerHandle_t shutdown_signal_timer;  // Timer to signal shutdown
static SemaphoreHandle_t shutdown_sema;    // Semaphore to signal shutdown


static void shutdown_signaler(TimerHandle_t xTimer)
{
    ESP_LOGI(TAG, "No data received for %d seconds, signaling shutdown", NO_DATA_TIMEOUT_SEC);
    xSemaphoreGive(shutdown_sema);
}

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

/**
 * @brief Initalize a WebSocket client by URI.
 *
 * This function initializes a WebSocket client using the provided URI.
 *
 * @param client: Pointer to the WebSocket client handle.
 * @param uri:    The URI of the WebSocket server.
 */
void ws_init_by_uri(esp_websocket_client_handle_t *client, const char* uri)
{
    if (*client != NULL)
    {
        ESP_LOGE(TAG, "The client has been initialized");
        return;
    }
    ws_cfg.uri = uri;
    // ws_cfg.port = 8888;  // Only used in development environment
    *client = esp_websocket_client_init(&ws_cfg);
}

void ws_register_event_handler(esp_websocket_client_handle_t *client, esp_websocket_event_id_t event, esp_event_handler_t event_handler)
{
    if (*client == NULL)
    {
        ESP_LOGE(TAG, "Please initialize the client first");
        return;
    }
    esp_websocket_register_events(*client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);
}

void ws_destroy_client(esp_websocket_client_handle_t *client)
{
    esp_websocket_client_destroy(*client);
    *client = NULL;
    ESP_LOGI(TAG, "Websocket client destroyed");
}

void ws_start(esp_websocket_client_handle_t *client)
{
    if (*client == NULL)
    {
        ESP_LOGE(TAG, "Please initialize the client first");
        return;
    }

    shutdown_signal_timer = xTimerCreate("Websocket shutdown timer", NO_DATA_TIMEOUT_SEC * 1000 / portTICK_PERIOD_MS,
                                         pdFALSE, NULL, shutdown_signaler);
    shutdown_sema = xSemaphoreCreateBinary();

    esp_websocket_client_start(*client);

    xTimerStart(shutdown_signal_timer, portMAX_DELAY);

    /* The following code is the logical code used for testing */
    ESP_LOGI(TAG, "Websocket client started");
    char data[32];
    int i = 0;
    while (i < 5)
    {
        if (esp_websocket_client_is_connected(*client))
        {
            int len = sprintf(data, "hello %04d", i++);
            ESP_LOGI(TAG, "Sending %s", data);
            esp_websocket_client_send_text(*client, data, len, portMAX_DELAY);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "Sending fragmented message");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    memset(data, 'a', sizeof(data));
    esp_websocket_client_send_text_partial(*client, data, sizeof(data), portMAX_DELAY);
    memset(data, 'b', sizeof(data));
    esp_websocket_client_send_cont_msg(*client, data, sizeof(data), portMAX_DELAY);
    esp_websocket_client_send_fin(*client, portMAX_DELAY);

    xSemaphoreTake(shutdown_sema, portMAX_DELAY);
    esp_websocket_client_close(*client, portMAX_DELAY);
    ESP_LOGI(TAG, "Websocket Stopped");
    ws_destroy_client(client);
}

void ws_test_app(void)
{
    ws_init_by_uri(&g_client, "ws://192.168.50.63");
    ws_register_event_handler(&g_client, WEBSOCKET_EVENT_ANY, websocket_event_handler);
    ws_start(&g_client);
}