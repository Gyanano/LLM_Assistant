#include "ws_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include <cJSON.h>
#include <string.h>

static const char *TAG = "WS_CLIENT";

esp_websocket_client_config_t ws_cfg = {};

// static TimerHandle_t shutdown_signal_timer;  // Timer to signal shutdown
// static SemaphoreHandle_t shutdown_sema;    // Semaphore to signal shutdown


// static void shutdown_signaler(TimerHandle_t xTimer)
// {
//     ESP_LOGI(TAG, "No data received for %d seconds, signaling shutdown", NO_DATA_TIMEOUT_SEC);
//     xSemaphoreGive(shutdown_sema);
// }

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
    esp_websocket_register_events(*client, event, event_handler, (void *)client);
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

    // shutdown_signal_timer = xTimerCreate("Websocket shutdown timer", NO_DATA_TIMEOUT_SEC * 1000 / portTICK_PERIOD_MS,
    //                                      pdFALSE, NULL, shutdown_signaler);
    // shutdown_sema = xSemaphoreCreateBinary();

    esp_websocket_client_start(*client);

    // xTimerStart(shutdown_signal_timer, portMAX_DELAY);

    // xSemaphoreTake(shutdown_sema, portMAX_DELAY);
    // esp_websocket_client_close(*client, portMAX_DELAY);
    // ESP_LOGI(TAG, "Websocket Stopped");
    // ws_destroy_client(client);
}

void ws_test_app(void)
{
    ESP_LOGI(TAG, "Start WebSocket test");
    // ws_init_by_uri(&g_client, "ws://192.168.50.63");
    // // ws_register_event_handler(&g_client, WEBSOCKET_EVENT_ANY, websocket_event_handler);
    // ws_start(&g_client);
}

void ws_test_chat(char* uri, char* json_str, int len)
{
    ESP_LOGI(TAG, "Start WebSocket test");
    // ws_init_by_uri(&g_client, uri);
    // // ws_register_event_handler(&g_client, WEBSOCKET_EVENT_ANY, websocket_event_handler);
    // ws_start(&g_client);
    // esp_websocket_client_send_text(g_client, json_str, len, portMAX_DELAY);
    // esp_websocket_client_close(g_client, portMAX_DELAY);
    // ws_destroy_client(&g_client);
}