#ifndef __BAIDU_STT_H__
#define __BAIDU_STT_H__

#include "esp_err.h"
#include "audio_event_iface.h"

#ifdef __cplusplus
extern "C" {
#endif

// define the stt buffer size
#define DEFAULT_STT_BUFFER_SIZE (8192)

typedef enum {
    ENCODING_LINEAR16 = 0,
} baidu_stt_encoding_t;

typedef struct baidu_stt* baidu_stt_handle_t;
typedef void (*baidu_stt_event_handle_t)(baidu_stt_handle_t stt);

typedef struct {
    int record_sample_rates;
    baidu_stt_encoding_t encoding;
    int buffer_size;
    baidu_stt_event_handle_t on_begin;
} baidu_stt_config_t;

baidu_stt_handle_t baidu_stt_init(baidu_stt_config_t *config);
esp_err_t baidu_stt_start(baidu_stt_handle_t stt);
esp_err_t baidu_stt_destroy(baidu_stt_handle_t stt);
char *baidu_stt_stop(baidu_stt_handle_t stt);
esp_err_t baidu_stt_set_listener(baidu_stt_handle_t stt, audio_event_iface_handle_t listener);

#ifdef __cplusplus
}
#endif

#endif