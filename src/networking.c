#include "networking.h"
#include "parser.h"
#include <curl/curl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE_URL "https://metromap.cityofmadison.com/bustime/api/v3/getpredictions"
#define MAX_RESPONSE_BYTES (2 * 1024 * 1024)
#ifndef REQUEST_TIMEOUT_SECONDS
#define REQUEST_TIMEOUT_SECONDS 10L
#endif

typedef struct {
    char *data;
    size_t size;
} ResponseBuffer;

static size_t callback(char *buffer, size_t itemsize, size_t nitems, void *userdata) {
    ResponseBuffer *response = userdata;
    if (itemsize && nitems > SIZE_MAX / itemsize) return 0;
    size_t bytes = itemsize * nitems;
    if (bytes > MAX_RESPONSE_BYTES - response->size) return 0;
    char *tmp = realloc(response->data, response->size + bytes + 1);
    if (!tmp) return 0;
    response->data = tmp;
    memcpy(response->data + response->size, buffer, bytes);
    response->size += bytes;
    response->data[response->size] = '\0';
    return bytes;
}

char *get_data(const char *url, size_t *out_size) {
    if (!out_size) return NULL;
    *out_size = 0;
    if (!url) return NULL;
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;
    ResponseBuffer response = {NULL, 0};
    CURLcode result;
    long status = 0;
#define SETOPT(option, value) do { \
    result = curl_easy_setopt(curl, option, value); \
    if (result != CURLE_OK) goto done; \
} while (0)
    SETOPT(CURLOPT_URL, url);
    SETOPT(CURLOPT_WRITEFUNCTION, callback);
    SETOPT(CURLOPT_WRITEDATA, &response);
    SETOPT(CURLOPT_USERAGENT, "departure-ticker/1.0");
    SETOPT(CURLOPT_CONNECTTIMEOUT, 5L);
    SETOPT(CURLOPT_TIMEOUT, REQUEST_TIMEOUT_SECONDS);
    SETOPT(CURLOPT_NOSIGNAL, 1L);
    SETOPT(CURLOPT_FAILONERROR, 1L);
    /* No redirects: do not forward the API key to another endpoint. TLS stays verified. */
    result = curl_easy_perform(curl);
    CURLcode info_result = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    if (result == CURLE_OK) result = info_result;
done:
#undef SETOPT
    curl_easy_cleanup(curl);
    if (result != CURLE_OK || status < 200 || status >= 300 || !response.size ||
        memchr(response.data, '\0', response.size)) {
        /* Do not log the URL, response body or curl error buffer: they can contain keys. */
        fprintf(stderr, "Request failed (curl=%d, HTTP=%ld).\n", (int)result, status);
        free(response.data);
        return NULL;
    }
    *out_size = response.size;
    return response.data;
}

char *build_url(const char *api_key, char *const stop_ids[], size_t n_stops,
                const char *feed) {
    if (!api_key || !*api_key || strlen(api_key) > 4096 || !stop_ids ||
        !n_stops || n_stops > MAX_STOPS || (feed && strlen(feed) > 256)) return NULL;
    char stops[MAX_STOPS * STOP_ID_SIZE] = "";
    for (size_t i = 0; i < n_stops; ++i) {
        if (!stop_ids[i] || !*stop_ids[i] || strlen(stop_ids[i]) >= STOP_ID_SIZE ||
            strspn(stop_ids[i], "0123456789") != strlen(stop_ids[i])) return NULL;
        if (i) strcat(stops, ",");
        strcat(stops, stop_ids[i]);
    }
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;
    char *key = curl_easy_escape(curl, api_key, 0);
    char *encoded_feed = curl_easy_escape(curl, feed ? feed : "", 0);
    char *url = NULL;
    if (key && encoded_feed) {
        const char *format = "%s?key=%s&format=json&unixTime=true&tmres=s&stpid=%s%s%s";
        const char *parameter = *encoded_feed ? "&rtpidatafeed=" : "";
        int size = snprintf(NULL, 0, format, BASE_URL, key, stops, parameter, encoded_feed);
        if (size >= 0) {
            url = malloc((size_t)size + 1);
            if (url) snprintf(url, (size_t)size + 1, format, BASE_URL, key, stops,
                              parameter, encoded_feed);
        }
    }
    curl_free(key);
    curl_free(encoded_feed);
    curl_easy_cleanup(curl);
    return url;
}
