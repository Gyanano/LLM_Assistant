#pragma once

#include "driver/i2c.h"
#include "driver/i2s_std.h"
#include "esp_system.h"
#include "esp_err.h"
#include "es8311.h"


/* I2C port and GPIOs */
#define I2C_NUM         (0)
#define I2C_SCL_IO      (GPIO_NUM_1)
#define I2C_SDA_IO      (GPIO_NUM_0)

/* I2S port and GPIOs */
#define I2S_NUM         (0)
#define I2S_MCK_IO      (GPIO_NUM_10)
#define I2S_BCK_IO      (GPIO_NUM_8)
#define I2S_WS_IO       (GPIO_NUM_12)
#define I2S_DO_IO       (GPIO_NUM_11)
#define I2S_DI_IO       (GPIO_NUM_7)

/* Other configurations */
#define I2S_RECV_BUF_SIZE   (5120)
#define I2S_SAMPLE_RATE     (8000)
#define I2S_MCLK_MULTIPLE   (384)
#define I2S_MCLK_FREQ_HZ    (I2S_SAMPLE_RATE * I2S_MCLK_MULTIPLE)
#define I2S_VOICE_VOLUME    50
#define I2S_MIC_GAIN        0

/* Error Type for Mic */
typedef enum {
    MIC_OK = 0,
    MIC_ERR_I2S_INIT_FAIL,
    MIC_ERR_DEVICE_INIT_FAIL,
    MIC_ERR_UNKNOWN,
} mic_err_t;


/**
 * @brief Initialize the microphone
 * 
 * @return mic_err_t 
 */
extern mic_err_t mic_init(void);