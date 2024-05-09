#include "bsp_key.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "KEY";

static QueueHandle_t gpio_evt_queue = NULL;
// define the handler function
static void (*on_key_up_handler)(void) = NULL;
static void (*on_key_down_handler)(void) = NULL;

static void gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    // printf("GPIO[%"PRIu32"] intr, val: %d\n", gpio_num, gpio_get_level(gpio_num));
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void on_key_down_handler_register(void *handler)
{
    on_key_down_handler = handler;
}

void on_key_up_handler_register(void *handler)
{
    on_key_up_handler = handler;
}

void key_init(void)
{
    gpio_config_t io_conf = {
        // .intr_type = GPIO_INTR_NEGEDGE,  // enable interrupt
        .intr_type = GPIO_INTR_ANYEDGE,  // enable interrupt
        .mode = GPIO_MODE_INPUT,         // set as output mode
        .pin_bit_mask = 1ULL << KEY_NUM, // bit mask of the pins that you want to set
        .pull_down_en = 0,               // disable pull-down mode
        .pull_up_en = 1,                 // enable pull-up mode, so the default state is high
    };

    // configure GPIO with the given settings
    gpio_config(&io_conf);

    // create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
}

void install_key_isr(void)
{
    // install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(KEY_NUM, gpio_isr_handler, (void *)KEY_NUM);
}

void key_scan_task(void *args)
{
    uint32_t io_num;
    ESP_LOGI(TAG, "[scan] Scan start");
    while (1)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            if (gpio_get_level(KEY_NUM) == 1)
            {
                // delay 20ms to debounce
                vTaskDelay(20 / portTICK_PERIOD_MS);
                if (gpio_get_level(KEY_NUM) == 1)
                {
                    ESP_LOGI(TAG, "[scan] Key up");
                    if (on_key_up_handler != NULL)
                    {
                        on_key_up_handler();
                    }
                }
            }
            else
            {
                // delay 20ms to debounce
                vTaskDelay(20 / portTICK_PERIOD_MS);
                if (gpio_get_level(KEY_NUM) == 0)
                {
                    ESP_LOGI(TAG, "[scan] Key down");
                    if (on_key_down_handler != NULL)
                    {
                        on_key_down_handler();
                    }
                }
            }
        }
    }
    vTaskDelete(NULL);
}
