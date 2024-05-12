#include "board.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_peripherals.h"
#include "periph_button.h"
#include "periph_wifi.h"
#include "audio.h"
#include "esp_efuse_table.h"
#include "esp_netif.h"
#include "audio_idf_version.h"
#include "nvs_flash.h"
#include "baidu_access_token.h"
#include "baidu_stt.h"
#include "baidu_tts.h"
#include "minimax_chat.h"

static const char *TAG = "MAIN";

char *baidu_access_token = NULL;

const char * minimax_key = "Bearer eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJHcm91cE5hbWUiOiLnq7noqIAiLCJVc2VyTmFtZSI6IuerueiogCIsIkFjY291bnQiOiIiLCJTdWJqZWN0SUQiOiIxNzgyMzYzOTA3MTA5NzAwMDU3IiwiUGhvbmUiOiIxODY3NTEwMjIyMyIsIkdyb3VwSUQiOiIxNzgyMzYzOTA3MTAxMzExNDQ5IiwiUGFnZU5hbWUiOiIiLCJNYWlsIjoiIiwiQ3JlYXRlVGltZSI6IjIwMjQtMDUtMTEgMTE6NDE6NDAiLCJpc3MiOiJtaW5pbWF4In0.ssCMdLz_oa57Ptj1-lK51IYgRzkfFoeHPe0VrIxFoOp0fgyaQ772TOHacfyNNDGGpZYqg-sqGGm8bEJ04gAORuRPQFIF-kXB7vJGJjtImG4BKC6nWJ1yrNb3o6dxgOVoqIADARaRMmjY54tn3JWXBCL_PnOtOAA33gAWlTrDK5oroEgXthk1tR1cP-5R6WYrhByiuIUkGCzz0LK1QqxSII5-7wo7Hmdd55NoTT_NuP53dl9jVUt10MAEeBjVTpXBolngUpbwQdt4ChBEWkzK00f1YIjhrokfGiA2IHuYBlO8sPtDBWLchiEi7NjyKkR1AaOudqnl1FrgnxFDgd0DRg";

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

    periph_wifi_cfg_t wifi_cfg = {
        .wifi_config.sta.ssid = CONFIG_ESP_WIFI_SSID,
        .wifi_config.sta.password = CONFIG_ESP_WIFI_PASSWORD,
    };
    esp_periph_handle_t wifi_handle = periph_wifi_init(&wifi_cfg);

    // Initialize Button peripheral
    periph_button_cfg_t btn_cfg = {
        .gpio_mask = (1ULL << get_input_mode_id()) | (1ULL << get_input_rec_id()),
    };
    esp_periph_handle_t button_handle = periph_button_init(&btn_cfg);

    // Start wifi & button peripheral
    esp_periph_start(set, button_handle);
    esp_periph_start(set, wifi_handle);

    periph_wifi_wait_for_connected(wifi_handle, portMAX_DELAY);

    if (baidu_access_token == NULL)
    {
        // Must freed `baidu_access_token` after used
        baidu_access_token = baidu_get_access_token(CONFIG_BAIDU_ACCESS_KEY, CONFIG_BAIDU_SECRET_KEY);
    }

    // init the baidu STT(Speech to Text)
    baidu_stt_config_t stt_config = {
        .record_sample_rates = 16000,
        .encoding = ENCODING_LINEAR16,
    };
    baidu_stt_handle_t stt = baidu_stt_init(&stt_config);

    // init the baidu TTS(Text to Speech)
    baidu_tts_config_t tts_config = {
        .playback_sample_rate = 16000,
    };
    baidu_tts_handle_t tts = baidu_tts_init(&tts_config);

    // set the listener
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);
    baidu_stt_set_listener(stt, evt);
    baidu_tts_set_listener(tts, evt);

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
        if (baidu_tts_check_event_finish(tts, &msg)) {
            ESP_LOGI(TAG, "[ * ] TTS Finish");
            es8311_pa_power(false); // 关闭音频
            continue;
        }

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
            baidu_tts_stop(tts);
            baidu_stt_start(stt);
        }
        else if (msg.cmd == PERIPH_BUTTON_RELEASE || msg.cmd == PERIPH_BUTTON_LONG_RELEASE)
        {
            char *original_text = baidu_stt_stop(stt);
            if (original_text == NULL) {
                minimax_content[0]=0; // 清空minimax 第1个字符写0就可以
                continue;
            }
            ESP_LOGI(TAG, "Original text = %s", original_text);

            char *answer = minimax_chat(original_text);
            if (answer == NULL)
            {
                continue;
            }
            ESP_LOGI(TAG, "minimax answer = %s", answer);
            es8311_pa_power(true); // 打开音频
            baidu_tts_start(tts, answer);
        }
    }
    ESP_LOGI(TAG, "Stop audio_pipeline");
    baidu_stt_destroy(stt);
    baidu_tts_destroy(tts);
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
    // esp_efuse_write_field_bit(ESP_EFUSE_VDD_SPI_AS_GPIO);
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