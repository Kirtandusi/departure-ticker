#include "networking.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

static char *global_json = NULL;
static size_t global_size = 0;


size_t callback(char *buffer, size_t itemsize, size_t nitems, void *other) {
    size_t bytes = itemsize * nitems;
    //collect data
    global_json = realloc(global_json, global_size + bytes + 1);
    if (!global_json) {
        fprintf(stderr, "Realloc failed\n");
        return 0;
    }
    memcpy(global_json + global_size, buffer, bytes);
    global_size += bytes;
    global_json[global_size] = '\0';
    printf("Received %zu bytes\n", bytes);
    return bytes;
    }
char *get_data() {
    CURL *curl = curl_easy_init();

    if(!curl) {
        fprintf(stderr, "Init failed\n");
        return NULL;
    }

    free(global_json);
    global_json = NULL;
    global_size = 0;

    char *api_key = getenv("API");
    if (!api_key) {
        fprintf(stderr, "API key not set\n");
        return NULL;
    }
    curl_easy_setopt(curl, CURLOPT_URL, api_key);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        curl_easy_strerror(res);
        return NULL;
    }
    

    curl_easy_cleanup(curl);
    return global_json;
}
