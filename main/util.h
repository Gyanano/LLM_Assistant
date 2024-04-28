#pragma once

/**
 * Only third-party libraries can be included. 
 * Do not inclue the custom library 
 * unless you know what you are doing.
 */
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "mbedtls/md.h"
#include "mbedtls/base64.h"


extern char* replace(char* str, const char* old, const char* new);
extern void get_gmttime(char *buf, int buf_len);
extern void hmac_sha256_calc(const char *secret, const char *src, char *output);
extern char *base64_encode(const char *src, size_t len);