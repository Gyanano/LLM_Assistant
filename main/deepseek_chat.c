#include "deepseek_chat.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "sdkconfig.h"
#include "audio_common.h"
#include "audio_hal.h"
#include "cJSON.h"

static const char *TAG = "DEEPSEEK_CHAT";

extern const char * deepseek_key;

#define PSOT_DATA    "{\
\"model\":\"deepseek-chat\",\
\"messages\":[\
{\"role\": \"system\", \"content\": \"You are a helpful assistant and you must answer in Chinese.\"},\
{\"role\": \"user\", \"content\": \"%s\"}\
],\
\"stream\":false\
}"


#define MAX_CHAT_BUFFER (2048)
char deepseek_content[2048]={0};

char *deepseek_chat(const char *text)
{
    char *response_text = NULL;
    char *post_buffer = NULL;
    char *data_buf = NULL; 

    esp_http_client_config_t config = {
        .url = "https://api.deepseek.com/chat/completions",  // 这里替换成自己的GroupId
        .buffer_size_tx = 512
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    int post_len = asprintf(&post_buffer, PSOT_DATA, text);
    
    if (post_buffer == NULL) {
        goto exit_translate;
    }

    // POST
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", deepseek_key);

    if (esp_http_client_open(client, post_len) != ESP_OK) {
        ESP_LOGE(TAG, "Error opening connection");
        goto exit_translate;
    }
    int write_len = esp_http_client_write(client, post_buffer, post_len);
    ESP_LOGI(TAG, "Need to write %d, written %d", post_len, write_len);

    int data_length = esp_http_client_fetch_headers(client);
    if (data_length <= 0) {
        data_length = MAX_CHAT_BUFFER;
    }
    data_buf = malloc(data_length + 1);
    if(data_buf == NULL) {
        goto exit_translate;
    }
    data_buf[data_length] = '\0';
    int rlen = esp_http_client_read(client, data_buf, data_length);
    data_buf[rlen] = '\0';
    ESP_LOGI(TAG, "read = %s", data_buf);

    cJSON *root = cJSON_Parse(data_buf);
    // response.choices[0].message.content is the answer
    int created = cJSON_GetObjectItem(root,"created")->valueint;
    if(created != 0)
    {
        // get the choices(array) and get the first one
        cJSON *choices = cJSON_GetObjectItem(root,"choices");
        cJSON *choice = cJSON_GetArrayItem(choices, 0);
        cJSON *message = cJSON_GetObjectItem(choice,"message");
        char *reply = cJSON_GetObjectItem(message,"content")->valuestring;
        // char *role = cJSON_GetObjectItem(message,"role")->valuestring;
        strcpy(deepseek_content, reply);
        response_text = deepseek_content;
        ESP_LOGI(TAG, "response_text(deepseek):%s", response_text);
    }

    cJSON_Delete(root);

exit_translate:
    free(post_buffer);
    free(data_buf);
    esp_http_client_cleanup(client);

    return response_text;
}
