#include "util.h"


/**
 * Replaces all occurrences of a substring in a string with another substring.
 *
 * @param str: The string in which to perform the replacement.
 * @param old: The substring to be replaced.
 * @param new: The substring to replace the occurrences of `old`.
 * @return A new dynamically allocated string with all occurrences of `old` replaced by `new`.
 *         The caller is responsible for freeing the memory allocated for the returned string.
 */
char* replace(char* str, const char* old, const char* new) {
    size_t old_len = strlen(old);
    size_t new_len = strlen(new);
    int i, count = 0;

    for (i = 0; str[i] != '\0'; i++) {
        if (strstr(&str[i], old) == &str[i]) {
            count++;
            i += old_len - 1;
        }
    }

    char* result = (char*)malloc(i + count * (new_len - old_len) + 1);
    i = 0;
    while (*str) {
        if (strstr(str, old) == str) {
            strcpy(&result[i], new);
            i += new_len;
            str += old_len;
        } else {
            result[i++] = *str++;
        }
    }

    result[i] = '\0';
    return result;
}


/**
 * Retrieves the current GMT time from local RTC and formats it into a string.
 *
 * This function retrieves the current time in GMT (Greenwich Mean Time) and formats it into a string
 * using the specified format ("%a, %d %b %Y %H:%M:%S GMT"). The formatted time string is then stored in
 * the provided buffer.
 *
 * @param buf: The buffer to store the formatted time string.
 * @param buf_len: The length of the buffer.
 */
void get_gmttime(char *buf, int buf_len)
{
    time_t now;
    struct tm *timeinfo;

    time(&now);
    timeinfo = gmtime(&now);
    strftime(buf, buf_len, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
}


/**
 * Calculates the HMAC-SHA256 hash of the given source data using the provided secret key.
 *
 * @param secret: The secret key used for HMAC calculation.
 * @param src: The source data to be hashed.
 * @param output: The output buffer to store the resulting HMAC-SHA256 hash, which must be at least 32 bytes long.
 */
void hmac_sha256_calc(const char *secret, const char *src, char *output)
{
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
    mbedtls_md_hmac_starts(&ctx, (const unsigned char*)secret, strlen(secret));
    mbedtls_md_hmac_update(&ctx, (const unsigned char*)src, strlen(src));
    mbedtls_md_hmac_finish(&ctx, (unsigned char*)output);
    mbedtls_md_free(&ctx);
}


/**
 * Encodes a given input buffer using Base64 encoding.
 *
 * @param src: The input buffer to be encoded.
 * @param len: The length of the input buffer.
 * @return A dynamically allocated string containing the Base64 encoded data.
 *         The caller is responsible for freeing the memory.
 */
char *base64_encode(const char *src, size_t len)
{
    size_t olen = 0;
    size_t out_len = ((len/3) + (len % 3 > 0 ? 1 : 0)) * 4;
    char *out = (char *)malloc(out_len + 1);
    mbedtls_base64_encode((unsigned char*)out, out_len + 1, &olen, (const unsigned char*)src, len);
    return out;
}