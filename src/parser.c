#define _XOPEN_SOURCE 700 // posix extension for strptime
#include "../include/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <time.h>


DepartureList *parse_json_predictions(char *json,
                                      char *stop_ids[],
                                      int n_stops) {
    cJSON *root = cJSON_Parse(json);
    // fprintf(stderr, "JSON parse failed:\n%s\n", json);
    if (!root) return NULL;

    cJSON *resp = cJSON_GetObjectItem(root, "bustime-response");
    if (!cJSON_IsObject(resp)) {
        DepartureList *list = calloc(1, sizeof(*list));
        list->items = calloc(n_stops, sizeof(BusDeparture));
        list->count = n_stops;

        for (int i = 0; i < n_stops; i++) {
            strncpy(list->items[i].stop_id, stop_ids[i], 7);
            list->items[i].stop_id[7] = '\0';
        }

        cJSON_Delete(root);
        return list;
    }

    cJSON *prd = cJSON_GetObjectItem(resp, "prd");
    if (!cJSON_IsArray(prd)) {
        DepartureList *list = calloc(1, sizeof(*list));
        list->items = calloc(n_stops, sizeof(BusDeparture));
        list->count = n_stops;

        for (int i = 0; i < n_stops; i++) {
            strncpy(list->items[i].stop_id, stop_ids[i], 7);
            list->items[i].stop_id[7] = '\0';
        }

        cJSON_Delete(root);
        return list;
    }


    DepartureList *list = malloc(sizeof(*list));
    if (!list) {
        cJSON_Delete(root);
        return NULL;
    }

    list->items = calloc(n_stops, sizeof(BusDeparture));
    if (!list->items) {
        free(list);
        cJSON_Delete(root);
        return NULL;
    }

    list->count = n_stops;

    for (int i = 0; i < n_stops; i++) {
        strncpy(list->items[i].stop_id, stop_ids[i], 7);
        list->items[i].stop_id[7] = '\0';
        list->items[i].arrival_unix_time = 0;
    }

   cJSON *item;
cJSON_ArrayForEach(item, prd) {
    cJSON *stpid_j = cJSON_GetObjectItem(item, "stpid"); //stop id
    cJSON *rt_j    = cJSON_GetObjectItem(item, "rt"); //actual identifier
    cJSON *prdtm_j = cJSON_GetObjectItem(item, "prdtm"); //time

    if (!cJSON_IsString(stpid_j) ||
        !cJSON_IsString(rt_j) ||
        !cJSON_IsString(prdtm_j))
        continue;

    char *stpid = stpid_j->valuestring;
    char *rt    = rt_j->valuestring;
    char *prdtm = prdtm_j->valuestring;

    struct tm tm = {0};
    if (!strptime(prdtm, "%Y%m%d %H:%M", &tm))
        continue;

    time_t ts = mktime(&tm);

    for (int k = 0; k < n_stops; k++) {
        if (strcmp(stpid, list->items[k].stop_id) != 0)
            continue;

        strncpy(list->items[k].route_name, rt, 7);
        list->items[k].route_name[7] = '\0';

        if (list->items[k].arrival_unix_time == 0 ||
            ts < list->items[k].arrival_unix_time)
            list->items[k].arrival_unix_time = ts;
        }
    }

    cJSON_Delete(root);
    return list;
}



void free_departure_list(DepartureList *list) {
    if (!list) return;
    free(list->items);
    free(list);
}
