#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "audio_hal.h"
#include "http_stream.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"
#include "baidu_stt.h"
#include "cJSON.h"

static const char *TAG = "BAIDU_STT";

extern char *baidu_access_token;  // defined in main.c

#define BAIDU_STT_ENDPOINT    "http://vop.baidu.com/server_api?dev_pid=1537&cuid=esp32c3&token=%s"
#define BAIDU_STT_TASK_STACK  (8*1024)


typedef struct baidu_stt {
    audio_pipeline_handle_t pipeline;
    char                    *buffer;
    audio_element_handle_t  i2s_reader;
    audio_element_handle_t  http_stream_writer;
    int                     sample_rates;
    int                     buffer_size;
    baidu_stt_encoding_t    encoding;
    char                    *response_text;
    baidu_stt_event_handle_t on_begin;
} baidu_stt_t;

char ask_text[256]={0};  // the buffer to store the text of the question

static esp_err_t _http_stream_writer_event_handle(http_stream_event_msg_t *msg)
{
    esp_http_client_handle_t http = (esp_http_client_handle_t)msg->http_client;
    baidu_stt_t *stt = (baidu_stt_t *)msg->user_data;

    static int total_write = 0;
    char len_buf[16];

    // the event of the pre-request, prepare the header
    if (msg->event_id == HTTP_STREAM_PRE_REQUEST) {
        ESP_LOGI(TAG, "[ + ] HTTP client HTTP_STREAM_PRE_REQUEST, lenght=%d", msg->buffer_len);

        esp_http_client_set_method(http, HTTP_METHOD_POST);
        esp_http_client_set_post_field(http, NULL, -1); // Chunk content
        esp_http_client_set_header(http, "Content-Type", "audio/pcm;rate=16000");
        esp_http_client_set_header(http, "Accept", "application/json");
        return ESP_OK;
    }

    // the event of the request, will enter the loop for many times
    if (msg->event_id == HTTP_STREAM_ON_REQUEST) {
        // write data
        int wlen = sprintf(len_buf, "%x\r\n", msg->buffer_len);
        if (esp_http_client_write(http, len_buf, wlen) <= 0) {
            return ESP_FAIL;
        }
        if (esp_http_client_write(http, msg->buffer, msg->buffer_len) <= 0) {
            return ESP_FAIL;
        }
        if (esp_http_client_write(http, "\r\n", 2) <= 0) {
            return ESP_FAIL;
        }
        total_write += msg->buffer_len;
        printf("\033[A\33[2K\rTotal bytes written: %d\n", total_write);
        return msg->buffer_len;  // return the length of the data written
    }

    /* Write End chunk */
    if (msg->event_id == HTTP_STREAM_POST_REQUEST) {
        ESP_LOGI(TAG, "[ + ] HTTP client HTTP_STREAM_POST_REQUEST, write end chunked marker");
        /* Finish chunked */
        if (esp_http_client_write(http, "0\r\n\r\n", 5) <= 0) {
            return ESP_FAIL;
        }
        return ESP_OK;
    }

    if (msg->event_id == HTTP_STREAM_FINISH_REQUEST) {

        int read_len = esp_http_client_read(http, (char *)stt->buffer, stt->buffer_size);
        ESP_LOGI(TAG, "[ + ] HTTP client HTTP_STREAM_FINISH_REQUEST, read_len=%d", read_len);
        if (read_len <= 0) {
            return ESP_FAIL;
        }
        if (read_len > stt->buffer_size - 1) {
            read_len = stt->buffer_size - 1;
        }
        stt->buffer[read_len] = 0;
        ESP_LOGI(TAG, "Got HTTP Response = %s", (char *)stt->buffer);

        // parse the response
        cJSON *root = cJSON_Parse(stt->buffer);
        int err_no = cJSON_GetObjectItem(root,"err_no")->valueint;

        if (err_no == 0)
        {
            cJSON *result = cJSON_GetObjectItem(root,"result");
            char *text = cJSON_GetArrayItem(result, 0)->valuestring;
            strcpy(ask_text, text);
            stt->response_text = ask_text;
            ESP_LOGI(TAG, "STT response: %s", ask_text);
        }
        else
        {
            stt->response_text = NULL;
        }
        cJSON_Delete(root);

        return ESP_OK;
    }
    return ESP_OK;
}

baidu_stt_handle_t baidu_stt_init(baidu_stt_config_t *config)
{
    // config the audio pipeline
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    baidu_stt_t *stt = calloc(1, sizeof(baidu_stt_t));
    AUDIO_MEM_CHECK(TAG, stt, return NULL);
    stt->pipeline = audio_pipeline_init(&pipeline_cfg);

    stt->buffer_size = config->buffer_size;
    if (stt->buffer_size <= 0)
    {
        stt->buffer_size = DEFAULT_STT_BUFFER_SIZE;
    }

    stt->buffer = (char *)malloc(stt->buffer_size);
    AUDIO_MEM_CHECK(TAG, stt->buffer, goto exit_stt_init);

    // config the i2s stream
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT_WITH_PARA(0, 16000, 16, AUDIO_STREAM_READER);
    i2s_cfg.std_cfg.slot_cfg.slot_mode = I2S_SLOT_MODE_MONO;
    i2s_cfg.std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    stt->i2s_reader = i2s_stream_init(&i2s_cfg);

    // config the http stream
    http_stream_cfg_t http_cfg = {
        .type = AUDIO_STREAM_WRITER,
        .event_handle = _http_stream_writer_event_handle,
        .user_data = stt,
        .task_stack = BAIDU_STT_TASK_STACK,
    };
    stt->http_stream_writer = http_stream_init(&http_cfg);
    stt->sample_rates = config->record_sample_rates;
    stt->encoding = config->encoding;
    stt->on_begin = config->on_begin;

    // connect the I2S stream and HTTP stream through a pipeline
    audio_pipeline_register(stt->pipeline, stt->http_stream_writer, "stt_http");
    audio_pipeline_register(stt->pipeline, stt->i2s_reader, "stt_i2s");
    const char *link_tag[2] = {"stt_i2s", "stt_http"};
    audio_pipeline_link(stt->pipeline, &link_tag[0], 2);

    // set the sample rate, bits and other parameters of the I2S stream
    i2s_stream_set_clk(stt->i2s_reader, config->record_sample_rates, 16, 1);

    return stt;
exit_stt_init:
    baidu_stt_destroy(stt);
    return NULL;
}

esp_err_t baidu_stt_destroy(baidu_stt_handle_t stt)
{
    if (stt == NULL) {
        return ESP_FAIL;
    }
    audio_pipeline_stop(stt->pipeline);
    audio_pipeline_wait_for_stop(stt->pipeline);
    audio_pipeline_terminate(stt->pipeline);
    audio_pipeline_remove_listener(stt->pipeline);
    audio_pipeline_deinit(stt->pipeline);
    free(stt->buffer);
    free(stt);
    return ESP_OK;
}

esp_err_t baidu_stt_set_listener(baidu_stt_handle_t stt, audio_event_iface_handle_t listener)
{
    if (listener) {
        audio_pipeline_set_listener(stt->pipeline, listener);
    }
    return ESP_OK;
}

esp_err_t baidu_stt_start(baidu_stt_handle_t stt)
{
    // generate the endpoint URL
    snprintf(stt->buffer, stt->buffer_size, BAIDU_STT_ENDPOINT, baidu_access_token); 
    // set the pipeline before running
    audio_element_set_uri(stt->http_stream_writer, stt->buffer);
    audio_pipeline_reset_items_state(stt->pipeline);
    audio_pipeline_reset_ringbuffer(stt->pipeline);
    // start the pipeline
    audio_pipeline_run(stt->pipeline);
    return ESP_OK;
}

char *baidu_stt_stop(baidu_stt_handle_t stt)
{
    audio_pipeline_stop(stt->pipeline);
    audio_pipeline_wait_for_stop(stt->pipeline);

    return stt->response_text;
}
