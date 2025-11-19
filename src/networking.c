#include "networking.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

char *get_data() {
    CURL *curl = curl_easy_init();

    if(!curl) {
        fprintf(stderr, "Init failed\n");
    }
    char *api_key = getenv("API");
    curl_easy_setopt(curl, CURLOPT_URL, api_key);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        free(curl);
        return NULL;
    }
    

    curl_easy_cleanup(curl);
    return EXIT_SUCCESS;
}
