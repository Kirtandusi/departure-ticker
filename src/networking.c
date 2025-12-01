#include "../include/networking.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *data;
    size_t size;
} ResponseBuffer;

size_t callback(char *buffer, size_t itemsize, size_t nitems, void *userdata) {
    size_t bytes = itemsize * nitems;
    ResponseBuffer *resp = (ResponseBuffer *)userdata;
    
    char *temp = realloc(resp->data, resp->size + bytes + 1);
    if (!temp) {
        fprintf(stderr, "Realloc failed\n");
        return 0;
    }
    
    resp->data = temp;
    memcpy(resp->data + resp->size, buffer, bytes); //copy memory from one location to another. 
    resp->size += bytes;
    resp->data[resp->size] = '\0';
    
    return bytes;
}

size_t global_pb_size = 0;

char *get_data() {
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Init failed\n");
        return NULL;
    }
    
    ResponseBuffer response = {NULL, 0};
    
    curl_easy_setopt(curl, CURLOPT_URL,
                     "https://metromap.cityofmadison.com/gtfsrt/trips");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        free(response.data);
        return NULL;
    }

    global_pb_size = response.size; 
    return response.data;
}
