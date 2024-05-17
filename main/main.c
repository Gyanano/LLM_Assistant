#include "board.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "audio_idf_version.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include "baidu_access_token.h"
#include "esp_peripherals.h"
#include "periph_button.h"
#include "nvs_flash.h"
#include "esp_efuse_table.h"
#include "baidu_tts.h"
#include "baidu_stt.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "esp_log.h"
#include <string.h>
#include "main.h"
#include "esp_lcd_touch_ft5x06.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "cJSON.h"
#include "zlib.h"
#include "esp_crt_bundle.h"
#include "esp_tls.h"

#if USE_MINIMAX
#include "minimax_chat.h"
#elif USE_DEEPSEEK
#include "deepseek_chat.h"
#endif

static const char *TAG = "MAIN";

char *baidu_access_token = NULL;

// if the USE_MODEL == MINIMAX, you need to set the minimax_key
#if USE_MINIMAX
const char *minimax_key = "Bearer eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJHcm91cE5hbWUiOiLnq7noqIAiLCJVc2VyTmFtZSI6IuerueiogCIsIkFjY291bnQiOiIiLCJTdWJqZWN0SUQiOiIxNzgyMzYzOTA3MTA5NzAwMDU3IiwiUGhvbmUiOiIxODY3NTEwMjIyMyIsIkdyb3VwSUQiOiIxNzgyMzYzOTA3MTAxMzExNDQ5IiwiUGFnZU5hbWUiOiIiLCJNYWlsIjoiIiwiQ3JlYXRlVGltZSI6IjIwMjQtMDUtMTEgMTE6NDE6NDAiLCJpc3MiOiJtaW5pbWF4In0.ssCMdLz_oa57Ptj1-lK51IYgRzkfFoeHPe0VrIxFoOp0fgyaQ772TOHacfyNNDGGpZYqg-sqGGm8bEJ04gAORuRPQFIF-kXB7vJGJjtImG4BKC6nWJ1yrNb3o6dxgOVoqIADARaRMmjY54tn3JWXBCL_PnOtOAA33gAWlTrDK5oroEgXthk1tR1cP-5R6WYrhByiuIUkGCzz0LK1QqxSII5-7wo7Hmdd55NoTT_NuP53dl9jVUt10MAEeBjVTpXBolngUpbwQdt4ChBEWkzK00f1YIjhrokfGiA2IHuYBlO8sPtDBWLchiEi7NjyKkR1AaOudqnl1FrgnxFDgd0DRg";
#elif USE_DEEPSEEK
const char *deepseek_key = "Bearer sk-82acc135ebc34f638a4b0ffa43a1e3fb";
#endif

#define LCD_HOST SPI2_HOST
#define LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)
#define LCD_BK_LIGHT_ON_LEVEL 0
#define LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define PIN_NUM_SCLK 3
#define PIN_NUM_MOSI 5
#define PIN_NUM_MISO -1
#define PIN_NUM_LCD_DC 6
#define PIN_NUM_LCD_RST -1
#define PIN_NUM_LCD_CS 4
#define PIN_NUM_BK_LIGHT 2
#define PIN_NUM_TOUCH_CS -1
#define LCD_H_RES 320
#define LCD_V_RES 240
#define LCD_CMD_BITS 8
#define LCD_PARAM_BITS 8
#define LVGL_TICK_PERIOD_MS 2

int ask_flag = 0;
int answer_flag = 0;
int lcd_clear_flag = 0;

static EventGroupHandle_t my_event_group;
#define WIFI_CONNECTED BIT0
#define GET_SNTP BIT1
#define GET_DAILYWEATHER BIT2
#define GET_RTWEATHER BIT3
#define GET_WEATHER BIT4

float bg_duty; // 液晶屏背光占空比 范围0~100%

extern void lv_main_page(void);
extern void lv_gui_start(void);

static esp_err_t nvs_init(void);

/**************************** LCD Backlight **********************************/
void lcd_brightness_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT, // Set duty resolution to 13 bits,
        .freq_hz = 5000,                      // Frequency in Hertz. Set frequency at 5 kHz
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = PIN_NUM_BK_LIGHT,
        .duty = 0, // Set duty
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

/**************************** LVGL handle func **********************************/
static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static void example_lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    uint16_t touchpad_x[1] = {0};
    uint16_t touchpad_y[1] = {0};
    uint8_t touchpad_cnt = 0;

    /* Read touch controller data */
    esp_lcd_touch_read_data(drv->user_data);

    /* Get coordinates */
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(drv->user_data, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

    if (touchpad_pressed && touchpad_cnt > 0)
    {
        data->point.x = touchpad_x[0];
        data->point.y = touchpad_y[0];
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// lvgl 的对象和全局变量
extern lv_obj_t *time_label;
extern lv_obj_t *label1;
extern lv_obj_t *label2;
extern int is_in_app;
extern char ask_text[256];

void value_update_cb(lv_timer_t *timer)
{
    static int cnt = 0;
    // 负责更新时间，但频次为每秒一次
    if (is_in_app == 1)
    {
        ESP_LOGI(TAG, "time update");
        if (++cnt > 10)
        {
            cnt = 0;
            time(&now);
            localtime_r(&now, &timeinfo);
            lv_label_set_text_fmt(time_label, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        }
    }
    else if (is_in_app == 2)
    {
        ESP_LOGI(TAG, "chatting");
        // 负责更新聊天内容
        if (ask_flag == 1)
        {
            lv_label_set_text_fmt(label1, "我：%s", ask_text);
            ask_flag = 0;
        }
        if (answer_flag == 1)
        {
#if USE_MINIMAX
            lv_label_set_text_fmt(label2, "AI：%s", minimax_content);
#elif USE_DEEPSEEK
            lv_label_set_text_fmt(label2, "AI：%s", deepseek_content);
#endif
            answer_flag = 0;
        }
        if (lcd_clear_flag == 1)
        {
            lcd_clear_flag = 0;
            lv_label_set_text(label1, "我：");
            lv_label_set_text(label2, "AI：");
        }
    }

}

static void main_page_task(void *pvParameters)
{
    ESP_LOGI(TAG, "main_page_task");
    xEventGroupWaitBits(my_event_group, GET_WEATHER, pdFALSE, pdFALSE, portMAX_DELAY);
    // vTaskDelay(pdMS_TO_TICKS(3000));
    lv_obj_clean(lv_scr_act());
    lv_main_page();
    lv_timer_create(value_update_cb, 100, NULL); // create a lv_timer for update the chat message

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

/**
 * 按键部分
 */
// TODO
/**
 * AI 聊天部分
 */
static void ai_chat_task(void *args)
{
    // 只有用户在菜单中选择了AI聊天功能，才会开启AI聊天任务
    ESP_LOGI(TAG, "ai_chat_task started");
    xEventGroupWaitBits(my_event_group, WIFI_CONNECTED, pdFALSE, pdFALSE, portMAX_DELAY);

    /* Peripheral Manager */
    // Initialize peripherals management
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    // Initialize Button peripheral
    periph_button_cfg_t btn_cfg = {
        .gpio_mask = (1ULL << get_input_mode_id()) | (1ULL << get_input_rec_id()),
    };
    esp_periph_handle_t button_handle = periph_button_init(&btn_cfg);

    // Start button peripheral
    esp_periph_start(set, button_handle);

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

    ESP_LOGI(TAG, "Chat app Ready!");
    printf("in app_main the min free stack size is %d \r\n", (int32_t)uxTaskGetStackHighWaterMark(NULL));

    while (1)
    {
        if (is_in_app != 2)
        {
            // 说明此时不是在聊天界面，则不再进行下面的逻辑
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        ESP_LOGW(TAG, "is_in_app is 2!");

        // FIX: 这个地方由于操作系统的现场保存和恢复似乎不能总是获取最新的值
        while (1)
        {
            audio_event_iface_msg_t msg;
            int temp_flag = is_in_app;
            if (temp_flag != 2)
            {
                // 说明不是在聊天界面，则不再进行下面的逻辑，且跳出循环
                ESP_LOGI(TAG, "is_in_app is not 2!");
                break;
            }
            
            if (audio_event_iface_listen(evt, &msg, portMAX_DELAY) != ESP_OK)
            {
                ESP_LOGW(TAG, "[ * ] Event process failed: src_type:%d, source:%p cmd:%d, data:%p, data_len:%d",
                         msg.source_type, msg.source, msg.cmd, msg.data, msg.data_len);
                continue;
            }

            // when the tts finish, close the speaker or its temperature will be high
            if (baidu_tts_check_event_finish(tts, &msg))
            {
                ESP_LOGI(TAG, "[ * ] TTS Finish");
                es8311_pa_power(false); // close the speaker
                continue;
            }

            // if the event is not from button, do nothing
            if (msg.source_type != PERIPH_ID_BUTTON)
            {
                continue;
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
                lcd_clear_flag = 1;
                baidu_stt_start(stt);
            }
            else if (msg.cmd == PERIPH_BUTTON_RELEASE || msg.cmd == PERIPH_BUTTON_LONG_RELEASE)
            {
                char *original_text = baidu_stt_stop(stt);
                if (original_text == NULL)
                {
#if USE_MINIMAX
                    minimax_content[0] = 0; // clear the minimax_content, just make the first element be 0
#elif USE_DEEPSEEK
                    deepseek_content[0] = 0; // clear the deepseek_content, just make the first element be 0
#endif
                    continue;
                }
                ESP_LOGI(TAG, "Original text = %s", original_text);
                ask_flag = 1;
#if USE_MINIMAX
                char *answer = minimax_chat(original_text);
#elif USE_DEEPSEEK
                char *answer = deepseek_chat(original_text);
#endif
                if (answer == NULL)
                {
                    continue;
                }
                ESP_LOGI(TAG, "Answer = %s", answer);
                answer_flag = 1;
                es8311_pa_power(true); // open the speaker
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
    }
    vTaskDelete(NULL);
}

/**
 * 获取天气部分
 */
#define MAX_HTTP_OUTPUT_BUFFER 2048
char g_temp[10];     // 实时天气温度
char g_humidity[10]; // 实时天气湿度

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer; // Buffer to store response of http request from event handler
    static int output_len;      // Stores number of bytes read
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            // If user_data buffer is configured, copy the response into the buffer
            int copy_len = 0;
            if (evt->user_data)
            {
                copy_len = MIN(evt->data_len, (2048 - output_len));
                if (copy_len)
                {
                    memcpy(evt->user_data + output_len, evt->data, copy_len);
                }
            }
            else
            {
                const int buffer_len = esp_http_client_get_content_length(evt->client);
                if (output_buffer == NULL)
                {
                    output_buffer = (char *)malloc(buffer_len);
                    output_len = 0;
                    if (output_buffer == NULL)
                    {
                        ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                        return ESP_FAIL;
                    }
                }
                copy_len = MIN(evt->data_len, (buffer_len - output_len));
                if (copy_len)
                {
                    memcpy(output_buffer + output_len, evt->data, copy_len);
                }
            }
            output_len += copy_len;
        }

        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        if (output_buffer != NULL)
        {
            // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
            // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
        if (err != 0)
        {
            ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        if (output_buffer != NULL)
        {
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        esp_http_client_set_header(evt->client, "From", "user@example.com");
        esp_http_client_set_header(evt->client, "Accept", "text/html");
        esp_http_client_set_redirection(evt->client);
        break;
    }
    return ESP_OK;
}

// GZIP解压函数
int gzDecompress(char *src, int srcLen, char *dst, int *dstLen)
{
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    strm.avail_in = srcLen;
    strm.avail_out = *dstLen;
    strm.next_in = (Bytef *)src;
    strm.next_out = (Bytef *)dst;

    int err = -1;
    err = inflateInit2(&strm, 31); // 初始化
    if (err == Z_OK)
    {
        printf("inflateInit2 err=Z_OK\n");
        err = inflate(&strm, Z_FINISH); // 解压gzip数据
        if (err == Z_STREAM_END)        // 解压成功
        {
            printf("inflate err=Z_OK\n");
            *dstLen = strm.total_out;
        }
        else // 解压失败
        {
            printf("inflate err=!Z_OK\n");
        }
        inflateEnd(&strm);
    }
    else
    {
        printf("inflateInit2 err! err=%d\n", err);
    }

    return err;
}

static void get_now_weather(void)
{
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    int https_status = 0;
    int64_t gzip_len = 0;
    int dstBufLen = 1024;
    char* dstBuf= (char*)malloc(1024);
    
    memset(dstBuf, 0, 1024);

    esp_http_client_config_t config = {
        .url = "https://devapi.qweather.com/v7/weather/now?location=101280102&key=52f795b81e974f46b5f0a533c0372ff1",
        .event_handler = _http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .user_data = local_response_buffer,        // Pass address of local buffer to get response
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    https_status = esp_http_client_get_status_code(client);
    gzip_len = esp_http_client_get_content_length(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %"PRIu64, https_status, gzip_len);
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);

    if (https_status == 200)
    {
        int ret = gzDecompress(local_response_buffer, gzip_len, dstBuf, &dstBufLen);

        if (Z_STREAM_END == ret) { /* 解压成功 */
            printf("decompress success\n");
            printf("dstBufLen = %d\n", dstBufLen);
            cJSON *root = cJSON_Parse(dstBuf);
            cJSON *now = cJSON_GetObjectItem(root,"now");
            char *temp = cJSON_GetObjectItem(now,"temp")->valuestring;
            char *humidity = cJSON_GetObjectItem(now,"humidity")->valuestring;
            strcpy(g_temp, temp);
            strcpy(g_humidity, humidity);
            ESP_LOGI(TAG, "地区：广州市番禺区");
            ESP_LOGI(TAG, "温度：%s", temp);
            ESP_LOGI(TAG, "湿度：%s", humidity);
            cJSON_Delete(root);
            free(dstBuf);
        }
        else {
            printf("decompress failed:%d\n", ret);
            free(dstBuf);
        }
    }
}

static void get_weather_task(void *pvParameters)
{
    xEventGroupWaitBits(my_event_group, GET_SNTP, pdFALSE, pdFALSE, portMAX_DELAY);
    get_now_weather();
    ESP_LOGI(TAG, "get_weather_task finish!");
    xEventGroupSetBits(my_event_group, GET_WEATHER);
    vTaskDelete(NULL);
}

/*******************
 ***** WIFI 部分*****
 ********************/
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < 5)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_ESP_WIFI_SSID,
            .password = CONFIG_ESP_WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

static void wifi_connect_task(void *pvParameters)
{
    wifi_init_sta();
    ESP_LOGI(TAG, "Successfully Connected to AP");
    xEventGroupSetBits(my_event_group, WIFI_CONNECTED);
    vTaskDelete(NULL);
}

/**
 * 时间部分
 */
time_t now;
struct tm timeinfo;

static void get_time_task(void *pvParameters)
{
    xEventGroupWaitBits(my_event_group, WIFI_CONNECTED, pdFALSE, pdFALSE, portMAX_DELAY);

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&config);
    // wait for time to be set
    int retry = 0;
    const int retry_count = 6;
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count)
    {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    }

    if (retry > 5)
    {
        esp_restart(); // 没有获取到时间的话 重启ESP32
    }

    esp_netif_sntp_deinit();
    // 设置时区
    setenv("TZ", "CST-8", 1);
    tzset();
    // 获取系统时间
    time(&now);
    localtime_r(&now, &timeinfo);
    xEventGroupSetBits(my_event_group, GET_SNTP);
    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_init());
    // ESP_EFUSE_VDD_SPI_AS_GPIO
    esp_efuse_write_field_bit(ESP_EFUSE_VDD_SPI_AS_GPIO);

    // initialize the audio board, including I2C, I2S
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);
    // initialize the codec(Speaker)
    es8311_codec_set_voice_volume(CONFIG_VOICE_VOLUME);
    es8311_pa_power(false); // close the PA when the board is initialized
    ESP_LOGI(TAG, "Initialize Speaker Success!");

    static lv_disp_draw_buf_t disp_buf;
    static lv_disp_drv_t disp_drv;

    /* Initialize SPI bus */
    ESP_LOGI(TAG, "Initialize SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_SCLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    /* Initialize the LCD */
    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_LCD_DC,
        .cs_gpio_num = PIN_NUM_LCD_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = example_notify_lvgl_flush_ready,
        .user_ctx = &disp_drv,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };

    /* Initialize LCD driver */
    ESP_LOGI(TAG, "Install ST7789 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    /* Initialize touch controller */
    esp_lcd_touch_handle_t tp = NULL;
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_V_RES,
        .y_max = LCD_H_RES,
        .rst_gpio_num = -1,
        .int_gpio_num = -1,
        .flags = {
            .swap_xy = 1,
            .mirror_x = 1,
            .mirror_y = 0,
        },
    };

    ESP_LOGI(TAG, "Initialize touch controller FT6336");
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, &tp));

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    lv_color_t *buf1 = heap_caps_malloc(LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
    lv_color_t *buf2 = heap_caps_malloc(LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2);

    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LCD_H_RES * 20);

    ESP_LOGI(TAG, "Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = example_lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    ESP_LOGI(TAG, "Install LVGL tick timer");

    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    static lv_indev_drv_t indev_drv; // Input device driver (Touch)
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.disp = disp;
    indev_drv.read_cb = example_lvgl_touch_cb;
    indev_drv.user_data = tp;

    lv_indev_drv_register(&indev_drv);

    /* Initialize the lcd backlight */
    lcd_brightness_init();
    // 设置占空比
    bg_duty = 0.5;                                                                         // 0.5表示占空比是50%
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 8191 * (1 - 0.5))); // 设置占空比 0.5表示占空比是50%
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));                // 更新背光

    my_event_group = xEventGroupCreate();

    lv_gui_start();

    xTaskCreate(get_weather_task, "get_weather_task", 8192, NULL, 5, NULL);
    xTaskCreate(get_time_task, "get_time_task", 2048, NULL, 5, NULL);
    xTaskCreate(wifi_connect_task, "wifi_connect_task", 4096, NULL, 5, NULL);
    xTaskCreate(main_page_task, "main_page_task", 8192, NULL, 5, NULL);
    // xTaskCreate(get_weather_task, "get_weather_task", 1024, NULL, 5, NULL);
    // xTaskCreate(sync_time_task, "sync_time_task", 512, NULL, 5, NULL);
    // AI chat task
    xTaskCreate(ai_chat_task, "ai_chat_task", 8192, NULL, 5, NULL);

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_timer_handler();
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