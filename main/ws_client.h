#pragma once

#include "esp_websocket_client.h"

#define NO_DATA_TIMEOUT_SEC 10

extern void ws_start(esp_websocket_client_handle_t *client);
extern void ws_init_by_uri(esp_websocket_client_handle_t *client, const char* uri);
extern void ws_register_event_handler(esp_websocket_client_handle_t *client, esp_websocket_event_id_t event, esp_event_handler_t event_handler);
extern void ws_destroy_client(esp_websocket_client_handle_t *client);
extern void ws_test_chat(char* uri, char* json_str, int len);