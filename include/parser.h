// parser.h
#ifndef PARSER_H
#define PARSER_H

typedef struct {
    char route_name[32];
    int departure_minutes;
} BusDeparture;

typedef struct {
    BusDeparture *items;
    int count;
} DepartureList;

DepartureList *parse_pb_to_list(char *pb, char *stop);
void free_departure_list(DepartureList *list);

#endif