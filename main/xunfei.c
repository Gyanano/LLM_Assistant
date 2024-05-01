#include "xunfei.h"
#include <stdio.h>
#include "esp_log.h"

static const char *TAG = "XUNFEI";
static time_t s_tmp_time;  // The variable for knowing current timestamp
static char s_xunfei_gmttime[32];  // Gmt time for generating Xunfei signature
static time_t xunfei_time = 0;  // Xunfei timestamp for update every 4 minutes
char xunfei_auth_url[352];  // The url for Xunfei authorization, the length is always 339
char *temp_p;  // Temporary pointer for memory allocation
static chat_history_t s_chat_history = {0};  // The chat history for the chat application
char chat_answer[2048] = {0};  // The answer to the user


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

/**
 * Calculates the length of the message buffer based on the role type of the message.
 *
 * @param msg: Pointer to the chat_msg_t structure representing the message.
 * @return The length of the message buffer(including the '\0').
 */
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

/**
 * Generates a message block string based on the given chat message.
 *
 * @param msg: The chat message to generate the message block string from.
 * @return The generated message block string, or NULL if the message is invalid.
 */
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

/**
 * Calculates the length of the history buffer.
 * 
 * @param history A pointer to the chat_history_t structure containing the chat history.
 * @return The length of the history buffer.
 */
int get_history_buf_length(chat_history_t *history)
{
    int len = strlen(SYSTEM_SETTING) + 30;  // 155 + 31 = 186
    for (int i = 0; i < history->count; i++)
    {
        len += (get_msg_buf_length(history->msg[i]) - 1);
        if (i < history->count - 1) len += 1;
    }
    len++; // add the null terminator --- '\0'
    return len;
}

/**
 * Generates a history string based on the chat history.
 * 
 * @param history: The chat history.
 * @param history_buf: The buffer to store the generated history string.
 * @param buf_len: The length of the history buffer.
 */
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

/**
 * Generates JSON parameters for a chat application.
 *
 * This function takes an app ID and model name as input and generates JSON parameters
 * for a chat application. The generated JSON parameters include the app ID, a unique
 * user identifier, and the chat history. The chat history is obtained from the
 * s_chat_history data structure and converted to a string format.
 *
 * @param app_id: The app ID for the chat application.
 * @param model_name: The model name for the chat application.
 * @return A pointer to the generated JSON parameters.
 * @note The caller is responsible for freeing the memory allocated for the JSON parameters.
 */
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

/**
 * @brief Updates the chat history with a new message.
 * 
 * This function updates the chat history with a new message. If the chat history is not full, 
 * the message is added to the end of the history. If the chat history is full, the oldest message 
 * is removed and the new message is added to the end.
 * 
 * @param msg: The message to be added to the chat history.
 */
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

/**
 * Frees the chat history by deallocating memory for each message.
 * 
 * This function iterates over the chat history array and frees the memory
 * allocated for each message. It also sets the corresponding array element
 * to NULL. Finally, it resets the count of chat history to 0.
 */
void free_chat_history(void)
{
    for (int i = 0; i < CHAT_MAX_HISTORY; i++)
    {
        if (s_chat_history.msg[i] != NULL)
        {
            free(s_chat_history.msg[i]);
            s_chat_history.msg[i] = NULL;
        }
    }
    s_chat_history.count = 0;
}

void clear_chat_answer(void)
{
    memset(chat_answer, 0, 2048);
}

void parse_chat_response(const char *data)
{
    // ESP_LOGI(TAG, "The response data is: %s", data);
    // parse the JSON data
    cJSON *root = cJSON_Parse(data);
    if (root == NULL)
    {
        ESP_LOGW(TAG, "Failed to parse JSON data!");
    }
    else
    {
        // parse the code
        cJSON *header = cJSON_GetObjectItem(root, "header");
        cJSON *code = cJSON_GetObjectItem(header, "code");
        if (code->valueint == 0)
        {
            cJSON *status = cJSON_GetObjectItem(header, "status");
            // if the header.status is 2, the response is the last one
            if (status->valueint == 2)
            {
                cJSON *payload = cJSON_GetObjectItem(root, "payload");
                cJSON *choices = cJSON_GetObjectItem(payload, "choices");
                cJSON *text = cJSON_GetObjectItem(choices, "text");
                cJSON *content = cJSON_GetArrayItem(text, 0);
                // cJSON *role = cJSON_GetObjectItem(content, "role");
                cJSON *content_str = cJSON_GetObjectItem(content, "content");
                // ESP_LOGI(TAG, "role: %s, content: %s", role->valuestring, content_str->valuestring);
                // concat the answer
                strcat(chat_answer, content_str->valuestring);
                ESP_LOGI(TAG, "The message is all received!");
            } 
            else
            {
                // ESP_LOGI(TAG, "The status is %d", status->valueint);
                cJSON *payload = cJSON_GetObjectItem(root, "payload");
                cJSON *choices = cJSON_GetObjectItem(payload, "choices");
                cJSON *text = cJSON_GetObjectItem(choices, "text");
                cJSON *content = cJSON_GetArrayItem(text, 0);
                // cJSON *role = cJSON_GetObjectItem(content, "role");
                cJSON *content_str = cJSON_GetObjectItem(content, "content");
                // ESP_LOGI(TAG, "role: %s, content: %s", role->valuestring, content_str->valuestring);
                // concat the answer
                strcat(chat_answer, content_str->valuestring);
            }
        }
        else
        {
            cJSON *message = cJSON_GetObjectItem(header, "message");
            ESP_LOGE(TAG, "The code is %d, request failed!\nThe error is: \"%s\"", code->valueint, message->valuestring);
        }
        cJSON_Delete(root);
    }
}



void update_auth_url(app_mode_t mode)
{
    xunfei_update_time();
    switch (mode)
    {
    case MODE_CHAT:
        generate_url(CHAT_URL, CHAT_HOST, CHAT_PATH);
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