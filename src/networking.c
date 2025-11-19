#include "networking.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
//https://maps.cityofmadison.com/arcgis/rest/services/Public/OPEN_DATA_TRANS/MapServer/18/query?where=1%3D1&outFields=*&outSR=4326&f=json

int get_data() {
    CURL *curl = curl_easy_init();

    if(!curl) {
        fprintf(stderr, "Init failed\n");
    }

    curl_easy_setopt(curl, CURLOPT_URL, "https://maps.cityofmadison.com/arcgis/rest/services/Public/OPEN_DATA_TRANS/MapServer/18/query?where=1%3D1&outFields=*&outSR=4326&f=json");
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
    

    curl_easy_cleanup(curl);
    return EXIT_SUCCESS;
}
