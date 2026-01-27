#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    char stop_id[8];
    char route_name[8];
    int64_t arrival_unix_time;
} BusDeparture;

typedef struct {
    BusDeparture *items;
    size_t count;
} DepartureList;

DepartureList *parse_json_predictions(char *json,
                                      char *stop_ids[],
                                      int n_stops);

void free_departure_list(DepartureList *list);

#endif
