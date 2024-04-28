#include "xunfei.h"
#include <stdio.h>
#include "esp_log.h"

static const char *TAG = "XUNFEI";
static time_t s_tmp_time;  // The variable for knowing current timestamp
static char s_xunfei_gmttime[32];  // Gmt time for generating Xunfei signature
time_t xunfei_time = 0;  // Xunfei timestamp for update every 4 minutes
char xunfei_auth_url[352];  // The url for Xunfei authorization, the length is always 339
char *temp_p;  // Temporary pointer for memory allocation


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

void free_temp_p(void)
{
    if (temp_p != NULL)
    {
        free(temp_p);
        temp_p = NULL;
    }
}

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