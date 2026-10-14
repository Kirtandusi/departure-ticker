#define _XOPEN_SOURCE 700
#include "parser.h"
#include <cjson/cJSON.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

int init_transit_timezone(void) {
    if (setenv("TZ", "America/Chicago", 1) != 0) return -1;
    tzset();
    return 0;
}

static int parse_timestamp(const cJSON *value, int64_t *result) {
    /* unixTime=true avoids ambiguity during the fall DST transition. */
    double milliseconds = 0;
    if (cJSON_IsNumber(value)) {
        milliseconds = value->valuedouble;
    } else if (cJSON_IsString(value)) {
        const char *text = value->valuestring;
        size_t len = strlen(text);
        if (len == 14 || len == 17) {
            const char *format = len == 14 ? "%Y%m%d %H:%M" : "%Y%m%d %H:%M:%S";
            struct tm parsed = {0};
            parsed.tm_isdst = -1;
            char *end = strptime(text, format, &parsed);
            if (end && *end == '\0') {
                time_t timestamp = mktime(&parsed);
                char normalized[32];
                /* Reject impossible dates and nonexistent spring-forward times. */
                if (timestamp == (time_t)-1 ||
                    !strftime(normalized, sizeof(normalized), format, &parsed) ||
                    strcmp(normalized, text) != 0) return -1;
                *result = (int64_t)timestamp;
                return 0;
            }
        }
        if (!len || strspn(text, "0123456789") != len) return -1;
        char *end;
        errno = 0;
        milliseconds = strtod(text, &end);
        if (errno || *end) return -1;
    } else {
        return -1;
    }
    /* Bound before converting to an integer (through year 9999). */
    if (!isfinite(milliseconds) || milliseconds < 100000000000.0 ||
        milliseconds > 253402300799999.0 || floor(milliseconds) != milliseconds)
        return -1;
    *result = (int64_t)(milliseconds / 1000.0);
    return 0;
}

static int empty_service_error(const cJSON *error) {
    const cJSON *message = cJSON_GetObjectItemCaseSensitive(error, "msg");
    if (!cJSON_IsString(message)) return 0;
    return strcasecmp(message->valuestring, "No arrival times") == 0 ||
           strcasecmp(message->valuestring, "No service scheduled") == 0 ||
           strcasecmp(message->valuestring, "No predictions available") == 0;
}

static int unavailable(const cJSON *item) {
    const cJSON *dyn = cJSON_GetObjectItemCaseSensitive(item, "dyn");
    int code = 0;
    if (cJSON_IsNumber(dyn)) code = dyn->valueint;
    else if (cJSON_IsString(dyn)) {
        const char *excluded[] = {"1", "4", "16", "17", "18"};
        for (size_t i = 0; i < sizeof(excluded) / sizeof(*excluded); ++i)
            if (strcmp(dyn->valuestring, excluded[i]) == 0) return 1;
    }
    return code == 1 || code == 4 || code == 16 || code == 17 || code == 18;
}

DepartureList *parse_json_predictions(const char *json,
                                      char *const stop_ids[],
                                      size_t n_stops, time_t now) {
    if (!json || !stop_ids || !n_stops || n_stops > MAX_STOPS) return NULL;
    for (size_t i = 0; i < n_stops; ++i) {
        if (!stop_ids[i] || !*stop_ids[i] || strlen(stop_ids[i]) >= STOP_ID_SIZE)
            return NULL;
    }
    cJSON *root = cJSON_ParseWithOpts(json, NULL, 1);
    if (!root) return NULL;
    const cJSON *resp = cJSON_GetObjectItemCaseSensitive(root, "bustime-response");
    const cJSON *prd = cJSON_GetObjectItemCaseSensitive(resp, "prd");
    const cJSON *errors = cJSON_GetObjectItemCaseSensitive(resp, "error");
    DepartureList *list = NULL;
    if (!cJSON_IsObject(resp)) goto done;

    int empty_response = 0;
    if (errors) {
        if (!cJSON_IsArray(errors)) goto done;
        const cJSON *error;
        cJSON_ArrayForEach(error, errors) {
            /* Never turn authentication, invalid-stop or quota errors into "no buses". */
            if (!empty_service_error(error)) goto done;
            empty_response = 1;
        }
    }
    if (prd ? !cJSON_IsArray(prd) : !empty_response) goto done;

    list = calloc(1, sizeof(*list));
    if (!list) goto done;
    list->items = calloc(n_stops, sizeof(*list->items));
    if (!list->items) {
        free(list);
        list = NULL;
        goto done;
    }
    list->count = n_stops;
    for (size_t i = 0; i < n_stops; ++i)
        snprintf(list->items[i].stop_id, STOP_ID_SIZE, "%s", stop_ids[i]);

    const cJSON *item;
    cJSON_ArrayForEach(item, prd) {
        const cJSON *stop = cJSON_GetObjectItemCaseSensitive(item, "stpid");
        const cJSON *route = cJSON_GetObjectItemCaseSensitive(item, "rt");
        const cJSON *arrival = cJSON_GetObjectItemCaseSensitive(item, "prdtm");
        int64_t timestamp;
        if (!cJSON_IsString(stop) || !cJSON_IsString(route) ||
            !*route->valuestring || strlen(route->valuestring) >= ROUTE_NAME_SIZE ||
            parse_timestamp(arrival, &timestamp) != 0) {
            /* Malformed predictions are an error, not evidence of no service. */
            free_departure_list(list);
            list = NULL;
            goto done;
        }
        if (unavailable(item) || timestamp + 59 < (int64_t)now ||
            timestamp > (int64_t)now + 12 * 60 * 60) continue;
        for (size_t k = 0; k < n_stops; ++k) {
            BusDeparture *departure = &list->items[k];
            if (strcmp(stop->valuestring, departure->stop_id) != 0) continue;
            if (!departure->arrival_unix_time || timestamp < departure->arrival_unix_time) {
                departure->arrival_unix_time = timestamp;
                snprintf(departure->route_name, ROUTE_NAME_SIZE, "%s", route->valuestring);
            }
        }
    }
done:
    cJSON_Delete(root);
    return list;
}

void free_departure_list(DepartureList *list) {
    if (!list) return;
    free(list->items);
    free(list);
}
