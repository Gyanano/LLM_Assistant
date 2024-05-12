#include "bsp_key.h"

static const char *TAG = "KEY";

esp_periph_handle_t key_init(void)
{
    // Initialize Button peripheral
    periph_button_cfg_t btn_cfg = {
        .gpio_mask = (1ULL << get_input_mode_id()) | (1ULL << get_input_rec_id()),
    };
    esp_periph_handle_t button_handle = periph_button_init(&btn_cfg);
}
