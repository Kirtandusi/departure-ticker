#define _POSIX_C_SOURCE 200809L
#include "config.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *trim(char *text) {
    while (isspace((unsigned char)*text)) ++text;
    char *end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return text;
}

int load_dotenv(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) return errno == ENOENT ? 0 : -1;
    char line[8192];
    int result = 0;
    while (fgets(line, sizeof(line), file)) {
        if (!strchr(line, '\n') && !feof(file)) { result = -1; break; }
        char *key = trim(line);
        if (!*key || *key == '#') continue;
        if (strncmp(key, "export ", 7) == 0) key = trim(key + 7);
        char *equals = strchr(key, '=');
        if (!equals) { result = -1; break; }
        *equals = '\0';
        key = trim(key);
        char *value = trim(equals + 1);
        if (!(isalpha((unsigned char)*key) || *key == '_') ||
            strspn(key, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") != strlen(key)) {
            result = -1; break;
        }
        size_t length = strlen(value);
        if (*value == '\'' || *value == '"') {
            if (length < 2 || value[length - 1] != *value) { result = -1; break; }
            value[length - 1] = '\0';
            ++value;
        }
        /* Shell/environment settings take precedence; never execute substitutions. */
        if (setenv(key, value, 0) != 0) { result = -1; break; }
    }
    if (ferror(file)) result = -1;
    fclose(file);
    return result;
}

int parse_stops(const char *text, char storage[MAX_STOPS][STOP_ID_SIZE],
                char *stops[MAX_STOPS], size_t *count) {
    *count = 0;
    if (!text || !*text) return -1;
    do {
        const char *comma = strchr(text, ',');
        size_t length = comma ? (size_t)(comma - text) : strlen(text);
        if (!length || length >= STOP_ID_SIZE || *count == MAX_STOPS ||
            strspn(text, "0123456789") != length) return -1;
        memcpy(storage[*count], text, length);
        storage[*count][length] = '\0';
        for (size_t i = 0; i < *count; ++i)
            if (strcmp(storage[i], storage[*count]) == 0) return -1;
        stops[*count] = storage[*count];
        ++*count;
        if (!comma) return 0;
        text = comma + 1;
    } while (*text);
    return -1;
}

int poll_interval(const char *text, unsigned *seconds) {
    if (!text) { *seconds = 15; return 0; }
    if (!*text || strspn(text, "0123456789") != strlen(text)) return -1;
    char *end;
    errno = 0;
    unsigned long value = strtoul(text, &end, 10);
    if (errno || *end || value < 15 || value > 3600) return -1;
    *seconds = (unsigned)value;
    return 0;
}
