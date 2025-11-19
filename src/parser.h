// parser.h
#ifndef PARSER_H
#define PARSER_H

typedef struct {
    char route_name[2];
    int departure_minutes;
} BusDeparture;

typedef struct {
    BusDeparture *items;
    int count;
} DepartureList;

DepartureList *parse_json(char *json, char *stop);
void free_departure_list(DepartureList *list);

#endif
