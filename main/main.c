#include "board.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "audio.h"
#include "esp_efuse_table.h"
#include "esp_netif.h"
#include "audio_idf_version.h"
#include "nvs_flash.h"
#include "baidu_access_token.h"
#include "bsp_key.h"
#include "bsp_wifi.h"
#include "ws_client.h"
#include "util.h"
#include "xunfei_chat.h"
#include "baidu_stt.h"

static const char *TAG = "MAIN";

esp_websocket_client_handle_t g_client = NULL;

char *baidu_access_token = NULL;

// static EventGroupHandle_t my_event_group;
// #define ALL_REDAY BIT0

// static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static esp_err_t nvs_init(void);

static void ai_chat_task(void *args)
{
    ESP_LOGI(TAG, "ai_chat_task started");

    /* Peripheral Manager */
    // Initialize peripherals management
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    // periph_cfg.task_stack = 8*1024;
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    // // initialize the button and wifi peripheral
    esp_periph_handle_t button_handle = key_init();
    esp_periph_handle_t wifi_handle = wifi_init();

    // Start button and wifi peripheral
    esp_periph_start(set, button_handle);
    esp_periph_start(set, wifi_handle);

    periph_wifi_wait_for_connected(wifi_handle, portMAX_DELAY);

    if (baidu_access_token == NULL)
    {
        ESP_LOGI(TAG, "baidu_access_token is NULL, get it now");
        // Must freed `baidu_access_token` after used
        baidu_access_token = baidu_get_access_token(CONFIG_BAIDU_ACCESS_KEY, CONFIG_BAIDU_SECRET_KEY);
    }
    ESP_LOGI(TAG, "baidu_access_token: %s", baidu_access_token);

    // init the baidu STT(Speech to Text)
    baidu_stt_config_t stt_config = {
        .record_sample_rates = 16000,
        .encoding = ENCODING_LINEAR16,
    };
    baidu_stt_handle_t stt = baidu_stt_init(&stt_config);
    ESP_LOGI(TAG, "baidu_stt_init finished");

    // set the listener
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);
    baidu_stt_set_listener(stt, evt);

    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);

    ESP_LOGI(TAG, "All Ready!");
    // set the event, to update the screen display
    // xEventGroupSetBits(my_event_group, ALL_REDAY);

    while (1)
    {
        audio_event_iface_msg_t msg;
        if (audio_event_iface_listen(evt, &msg, portMAX_DELAY) != ESP_OK)
        {
            ESP_LOGW(TAG, "[ * ] Event process failed: src_type:%d, source:%p cmd:%d, data:%p, data_len:%d",
                     msg.source_type, msg.source, msg.cmd, msg.data, msg.data_len);
            continue;
        }

        // when the tts finish, close the speaker or its temperature will be high
        // if (baidu_tts_check_event_finish(tts, &msg)) {
        //     ESP_LOGI(TAG, "[ * ] TTS Finish");
        //     es8311_pa_power(false); // 关闭音频
        //     continue;
        // }

        // if the event is not from button, do nothing
        if (msg.source_type != PERIPH_ID_BUTTON)
        {
            continue;
        }

        // if the program comes here, it means the event is from button,
        // check the button id, if it is other button, break the loop and stop the task
        if ((int)msg.data == get_input_mode_id())
        {
            break;
        }

        // if the button is not the record button, do nothing
        if ((int)msg.data != get_input_rec_id())
        {
            continue;
        }

        // if the program comes here, it means the button is the record button,
        // and check the event is from button up or down
        if (msg.cmd == PERIPH_BUTTON_PRESSED)
        {
            // stop the tts
            // baidu_tts_stop(tts);
            // lcd_clear_flag = 1;
            baidu_stt_start(stt);
        }
        else if (msg.cmd == PERIPH_BUTTON_RELEASE || msg.cmd == PERIPH_BUTTON_LONG_RELEASE)
        {
            // if the event is from button release, stop the tts
            char *original_text = baidu_stt_stop(stt);
            // if (original_text == NULL) {
            //     minimax_content[0]=0; // clear the minimax answer
            //     continue;
            // }
            ESP_LOGI(TAG, "Original text = %s", original_text);
            // ask_flag = 1;

            // char *answer = minimax_chat(original_text);
            // if (answer == NULL)
            // {
            //     continue;
            // }
            // ESP_LOGI(TAG, "minimax answer = %s", answer);
            // answer_flag = 1;
            // es8311_pa_power(true); // open the speaker
            // baidu_tts_start(tts, answer);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "Stop audio_pipeline");
    baidu_stt_destroy(stt);
    /* Stop all periph before removing the listener */
    esp_periph_set_stop_all(set);
    audio_event_iface_remove_listener(esp_periph_set_get_event_iface(set), evt);

    /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    audio_event_iface_destroy(evt);
    esp_periph_set_destroy(set);
    vTaskDelete(NULL);
}

void app_main(void)
{
    // ESP_EFUSE_VDD_SPI_AS_GPIO
    esp_efuse_write_field_bit(ESP_EFUSE_VDD_SPI_AS_GPIO);
    ESP_ERROR_CHECK(nvs_init());

    ESP_ERROR_CHECK(esp_netif_init());

    // initial the audio module
    ESP_ERROR_CHECK(audio_init());
    ESP_LOGI(TAG, "audio init success");

    // AI chat task
    xTaskCreate(ai_chat_task, "ai_chat_task", 8192, NULL, 5, NULL);

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        // lv_timer_handler();
    }
}

// static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
// {
//     esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
//     switch (event_id)
//     {
//     case WEBSOCKET_EVENT_CONNECTED: // 1
//         ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
//         clear_chat_answer();
//         break;
//     case WEBSOCKET_EVENT_DISCONNECTED:
//         ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
//         break;
//     case WEBSOCKET_EVENT_DATA: // 3
//         parse_chat_response((const char *)data->data_ptr);
//         break;
//     case WEBSOCKET_EVENT_ERROR:
//         ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
//         break;
//     }
// }

/* System Configuration */
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