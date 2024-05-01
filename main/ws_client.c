#include "ws_client.h"
#include "esp_log.h"

#include <cJSON.h>
#include <string.h>

static const char *TAG = "WS_CLIENT";


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
    esp_websocket_client_config_t ws_cfg = {};
    *client = esp_websocket_client_init(&ws_cfg);
    esp_websocket_client_set_uri(*client, uri);
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
    esp_websocket_client_start(*client);
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