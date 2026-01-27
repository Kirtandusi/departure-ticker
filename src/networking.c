#include "../include/networking.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define BASE_URL "https://metromap.cityofmadison.com/bustime/api/v3/getpredictions"
typedef struct {
    char *data;
    size_t size;
} ResponseBuffer;

static size_t callback(char *buffer, size_t itemsize,
                       size_t nitems, void *userdata) {
    size_t bytes = itemsize * nitems;
    ResponseBuffer *resp = userdata;

    char *tmp = realloc(resp->data, resp->size + bytes + 1);
    if (!tmp) return 0;

    resp->data = tmp;
    memcpy(resp->data + resp->size, buffer, bytes);
    resp->size += bytes;
    resp->data[resp->size] = '\0';

    return bytes;
}


char *get_data(const char *url, size_t *out_size) {
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    ResponseBuffer response = {NULL, 0};

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");

    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    //fprintf(stderr, "REQUEST URL:\n%s\n", url);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(response.data);
        return NULL;
    }

    *out_size = response.size;
    return response.data;
}

char *build_url(const char *api_key,
                char *stop_ids[],
                size_t n_stops) {
    size_t bufsize = 512;
    char *url = malloc(bufsize);
    if (!url) return NULL;
    snprintf(url, bufsize,
             "%s?key=%s&format=json&stpid=",
             BASE_URL, api_key);

    for (size_t i = 0; i < n_stops; i++) {
        strcat(url, stop_ids[i]);
        if (i + 1 < n_stops)
            strcat(url, ",");
    }

    return url;
}

