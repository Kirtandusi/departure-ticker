#include "networking.h"
#include <assert.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    assert(argc == 2);
    assert(curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK);
    const char *paths[] = {"/ok", "/http-error", "/empty", "/redirect", "/large", "/nul", "/slow"};
    for (size_t i = 0; i < sizeof(paths) / sizeof(*paths); ++i) {
        char url[256];
        snprintf(url, sizeof(url), "%s%s", argv[1], paths[i]);
        size_t size = 123;
        char *body = get_data(url, &size);
        if (!i) assert(body && size == 2 && !strcmp(body, "{}"));
        else assert(body == NULL && size == 0);
        free(body);
    }
    curl_global_cleanup();
    puts("HTTP regressions passed (status, empty, redirect, size, NUL, timeout).");
    return 0;
}
