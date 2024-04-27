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