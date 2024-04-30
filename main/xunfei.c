#include "xunfei.h"
#include <stdio.h>
#include "esp_log.h"

static const char *TAG = "XUNFEI";
static time_t s_tmp_time;  // The variable for knowing current timestamp
static char s_xunfei_gmttime[32];  // Gmt time for generating Xunfei signature
time_t xunfei_time = 0;  // Xunfei timestamp for update every 4 minutes
char xunfei_auth_url[352];  // The url for Xunfei authorization, the length is always 339
char *temp_p;  // Temporary pointer for memory allocation
static chat_history_t s_chat_history;  // The chat history for the chat application


/**
 * @brief Updates the timestamp for Xunfei.
 * 
 * This function gets the current timestamp and updates the Xunfei timestamp every 4 minutes.
 * 
 * @note The Xunfei timestamp is updated only if 4 minutes have passed since the last update.
 */
static void xunfei_update_time(void)
{
    // Get current timestamp
    time(&s_tmp_time);
    // Update xunfei timestamp every 4 minutes
    if ((long int)xunfei_time + 240 < (long int)s_tmp_time)
    {
        xunfei_time = s_tmp_time;
        get_gmttime(s_xunfei_gmttime, 32);
    }
}

/**
 * @brief Frees the memory allocated for the `temp_p` pointer.
 * 
 * This function checks if the `temp_p` pointer is not NULL, and if so, frees the memory
 * allocated for it using the `free` function. After freeing the memory, the `temp_p` pointer
 * is set to NULL.
 */
void free_temp_p(void)
{
    if (temp_p != NULL)
    {
        free(temp_p);
        temp_p = NULL;
    }
}

/**
 * Generates a URL with authorization parameters for Xunfei API.
 *
 * @param url: The base URL for the API.
 * @param host: The host name.
 * @param path: The path of the API endpoint.
 */
static void generate_url(char *url, char *host, char *path)
{
    /* Get the base64 code of decrypted signature */
    char signature[256];
    sprintf(signature, "host: %s\ndate: %s\nGET %s HTTP/1.1", host, s_xunfei_gmttime, path);

    char hmac[32];
    hmac_sha256_calc(API_SECRET, signature, hmac);

    char decrypted_signature_base64[64];
    temp_p = base64_encode(hmac, 32);
    strcpy(decrypted_signature_base64, temp_p);
    free_temp_p();

    /* Generate the url */
    // make a copy of the GMT time string
    char replaced_date[50] = {0};  // The replaced date string has 45 bytes.
    memcpy(replaced_date, s_xunfei_gmttime, 32);
    // replace the three special characters of the GMT time
    temp_p = replace(replaced_date, ",", "%2C");
    strcpy(replaced_date, temp_p);
    free_temp_p();
    temp_p = replace(replaced_date, " ", "%20");
    strcpy(replaced_date, temp_p);
    free_temp_p();
    temp_p = replace(replaced_date, ":", "%3A");
    strcpy(replaced_date, temp_p);
    free_temp_p();
    // concatenate the url
    char auth_origin[256];
    char auth_base64[256];
    sprintf(auth_origin, "api_key=\"%s\", algorithm=\"hmac-sha256\", headers=\"host date request-line\", signature=\"%s\"", API_KEY, decrypted_signature_base64);
    temp_p = base64_encode(auth_origin, strlen(auth_origin));
    strcpy(auth_base64, temp_p);
    free_temp_p();
    sprintf(xunfei_auth_url, "%s?authorization=%s&date=%s&host=%s", url, auth_base64, replaced_date, host);
}

int get_msg_buf_length(chat_msg_t *msg)
{
    switch (msg->role_type)
    {
        case MSG_ROLE_SYSTEM:
        return 31 + msg->length;
        case MSG_ROLE_ASSISTANT:
        return 34 + msg->length;
        case MSG_ROLE_USER:
        return 29 + msg->length;
        default:
        ESP_LOGW(TAG, "Get the invalid role type when get message block length!");
        return 0;
    }
}

char* generate_msg_block_string(chat_msg_t *msg)
{
    // {"role":"","content":""}  // The length of this string is 24
    int len = get_msg_buf_length(msg);
    if (len == 0)
    {
        ESP_LOGW(TAG, "Ignore the invalid message block!");
        return NULL;
    }
    else
    {
        char *msg_block = (char *)malloc(len);
        sprintf(msg_block, "{\"role\":\"%s\",\"content\":\"%s\"}", msg->role, msg->content);
        return msg_block;
    }
}

int get_history_buf_length(chat_history_t *history)
{
    int len = strlen(SYSTEM_SETTING) + 31;
    for (int i = 0; i < history->count; i++)
    {
        len += (get_msg_buf_length(history->msg[i]) - 1);
        if (i < history->count - 1) len += 1;
    }
    len++;
    return len;
}

void generate_history_string(chat_history_t *history, char *history_buf, int buf_len)
{
    // clear the history buffer
    memset(history_buf, 0, buf_len);
    // first, add the system setting
    sprintf(history_buf, "{\"role\":\"system\",\"content\":\"%s\"}", SYSTEM_SETTING);
    if (history->count == 0)
    {
        ESP_LOGW(TAG, "The chat history is empty!");
        return;
    }
    strcat(history_buf, ",");
    
    // define the temporary pointer for the message block
    // if use the public pointer, the memory will be freed twice and cause the error
    char *block_p = NULL;
    for (int i = 0; i < history->count; i++)
    {
        block_p = generate_msg_block_string(history->msg[i]);
        if (block_p == NULL)
        {
            ESP_LOGW(TAG, "Ignore the invalid message block!(index: %d)", i);
            continue;
        }
        strcat(history_buf, block_p);
        free(block_p);
        if (block_p != NULL) block_p = NULL;
        if (i < history->count - 1) strcat(history_buf, ",");
    }
}

char *generate_json_params(const char* app_id, const char* model_name)
{
    // uid: The unique identifier of the user, can be any string.
    // {"header":{"app_id":"","uid":"esp32c3"},"parameter":{"chat":{"domain":"","temperature":0.5,"max_tokens":1024}},"payload":{"message":{"text":[]}}}
    int history_len = get_history_buf_length(&s_chat_history);
    temp_p = (char *)malloc(history_len);
    generate_history_string(&s_chat_history, temp_p, history_len);

    int len = 145 + strlen(app_id) + strlen(model_name) + history_len;
    char *json_params = (char *)malloc(len);
    sprintf(json_params, "{\"header\":{\"app_id\":\"%s\",\"uid\":\"esp32c3\"},\"parameter\":{\"chat\":{\"domain\":\"%s\",\"temperature\":0.5,\"max_tokens\":1024}},\"payload\":{\"message\":{\"text\":[%s]}}}", app_id, model_name, temp_p);
    free_temp_p();
    return json_params;
}

void update_chat_history(chat_msg_t *msg)
{
    if (s_chat_history.count < CHAT_MAX_HISTORY)
    {
        s_chat_history.msg[s_chat_history.count] = msg;
        s_chat_history.count++;
    }
    else
    {
        // The first version uses arrays to store chat records, and later implements storage through linked lists
        for (int i = 0; i < CHAT_MAX_HISTORY - 1; i++)
        {
            s_chat_history.msg[i] = s_chat_history.msg[i + 1];
        }
        s_chat_history.msg[CHAT_MAX_HISTORY - 1] = msg;
    }
}

void update_auth_url(app_mode_t mode)
{
    xunfei_update_time();
    switch (mode)
    {
    case MODE_CHAT:
        generate_url(CHAT_URL, CHAT_HOST, CHAT_PATH);
        ESP_LOGI(TAG, "The auth url is: %s", xunfei_auth_url);
        temp_p = generate_json_params(APP_ID, MODEL);
        ESP_LOGI(TAG, "The json params is: %s", temp_p);
        free_temp_p();
        // ESP_LOGI(TAG, "The auth url is: %s\nThe len of url is: %d", xunfei_auth_url, strlen(xunfei_auth_url));
        break;
    case MODE_STT:
        printf(STT_URL);
        break;
    case MODE_TTS:
        printf(TTS_URL);
        break;
    default:
        ESP_LOGE(TAG, "Get the invalid mode when update url!");
        break;
    }
}