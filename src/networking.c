#include "../include/networking.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    char *data;
    size_t size;
} ResponseBuffer;

static size_t callback(char *buffer, size_t itemsize,
                       size_t nitems, void *userdata) {
    size_t bytes = itemsize * nitems;
    ResponseBuffer *resp = userdata;

    char *tmp = realloc(resp->data, resp->size + bytes);
    if (!tmp) return 0;

    resp->data = tmp;
    memcpy(resp->data + resp->size, buffer, bytes);
    resp->size += bytes;

    return bytes;
}

char *get_data(size_t *out_size) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    ResponseBuffer response = {NULL, 0};

    curl_easy_setopt(curl, CURLOPT_URL,
        "https://metromap.cityofmadison.com/gtfsrt/trips");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(response.data);
        return NULL;
    }

    *out_size = response.size;
    return response.data;
}
