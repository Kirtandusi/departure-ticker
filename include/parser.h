#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#define MAX_STOPS 10
#define STOP_ID_SIZE 32
#define ROUTE_NAME_SIZE 32

typedef struct {
    char stop_id[STOP_ID_SIZE];
    char route_name[ROUTE_NAME_SIZE];
    int64_t arrival_unix_time;
} BusDeparture;

typedef struct {
    BusDeparture *items;
    size_t count;
} DepartureList;

/* Call once before starting display threads; legacy timestamps use Chicago time. */
int init_transit_timezone(void);

/* NULL means invalid data/API failure; an empty stop has arrival_unix_time == 0. */
DepartureList *parse_json_predictions(const char *json,
                                      char *const stop_ids[],
                                      size_t n_stops, time_t now);

void free_departure_list(DepartureList *list);

#endif
