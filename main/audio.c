#include "audio.h"
#include "board.h"

static const char *TAG = "AUDIO";
// static i2s_chan_handle_t tx_handle = NULL;
// static i2s_chan_handle_t rx_handle = NULL;


// static esp_err_t i2s_driver_init(void)
// {
//     i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM, I2S_ROLE_MASTER);
//     chan_cfg.auto_clear = true; // Auto clear the legacy data in the DMA buffer
//     ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle));
//     i2s_std_config_t std_cfg = {
//         .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(I2S_SAMPLE_RATE),
//         .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
//         .gpio_cfg = {
//             .mclk = I2S_MCK_IO,
//             .bclk = I2S_BCK_IO,
//             .ws = I2S_WS_IO,
//             .dout = I2S_DO_IO,
//             .din = I2S_DI_IO,
//             .invert_flags = {
//                 .mclk_inv = false,
//                 .bclk_inv = false,
//                 .ws_inv = false,
//             },
//         },
//     };
//     std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE;

//     ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle, &std_cfg));
//     ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));
//     ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));
//     ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
//     return ESP_OK;
// }

// static esp_err_t es8311_codec_init(void)
// {
//     /* Initialize I2C peripheral */
//     const i2c_config_t es_i2c_cfg = {
//         .sda_io_num = I2C_SDA_IO,
//         .scl_io_num = I2C_SCL_IO,
//         .mode = I2C_MODE_MASTER,
//         .sda_pullup_en = GPIO_PULLUP_ENABLE,
//         .scl_pullup_en = GPIO_PULLUP_ENABLE,
//         .master.clk_speed = 100000,
//     };
//     ESP_RETURN_ON_ERROR(i2c_param_config(I2C_NUM, &es_i2c_cfg), TAG, "config i2c failed");
//     ESP_RETURN_ON_ERROR(i2c_driver_install(I2C_NUM, I2C_MODE_MASTER, 0, 0, 0), TAG, "install i2c driver failed");

//     /* Initialize es8311 codec */
//     es8311_handle_t es_handle = es8311_create(I2C_NUM, ES8311_ADDRRES_0);
//     ESP_RETURN_ON_FALSE(es_handle, ESP_FAIL, TAG, "es8311 create failed");
//     const es8311_clock_config_t es_clk = {
//         .mclk_inverted = false,
//         .sclk_inverted = false,
//         .mclk_from_mclk_pin = true,
//         .mclk_frequency = I2S_MCLK_FREQ_HZ,
//         .sample_frequency = I2S_SAMPLE_RATE};

//     ESP_ERROR_CHECK(es8311_init(es_handle, &es_clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16));
//     ESP_RETURN_ON_ERROR(es8311_sample_frequency_config(es_handle, I2S_SAMPLE_RATE * I2S_MCLK_MULTIPLE, I2S_SAMPLE_RATE), TAG, "set es8311 sample frequency failed");
//     ESP_RETURN_ON_ERROR(es8311_voice_volume_set(es_handle, I2S_VOICE_VOLUME, NULL), TAG, "set es8311 volume failed");
//     ESP_RETURN_ON_ERROR(es8311_microphone_config(es_handle, false), TAG, "set es8311 microphone failed");
//     ESP_RETURN_ON_ERROR(es8311_microphone_gain_set(es_handle, I2S_MIC_GAIN), TAG, "set es8311 microphone gain failed");

//     return ESP_OK;
// }

/**
 * @brief Initialize the microphone
 * 
 * @return mic_err_t 
 */
esp_err_t audio_init(void)
{
    // initialize the audio board
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);

    // initialize the codec(Speaker)
    es8311_codec_set_voice_volume(AUDIO_VOICE_VOLUME);
    es8311_pa_power(false);  // close the PA when the board is initialized

    return ESP_OK;
}