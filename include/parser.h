#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    char stop_id[8];       // add this line
    char route_name[8];
    int64_t arrival_unix_time;
} BusDeparture;


typedef struct {
    BusDeparture *items;
    size_t count;
} DepartureList;

DepartureList *parse_pb_to_list(char *pb, size_t pb_size,
                                char *stop_ids[], char *route_letters[], size_t n_stops);
void free_departure_list(DepartureList *list);

#endif
