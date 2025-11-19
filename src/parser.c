#include "parser.h"
#include <stdio.h>

DepartureList *parse_json(char *json, char *stop) {
    if (!json) {
        printf("No JSON to parse\n");
        return;
    }

    // TODO: implement parsing logic

    //return bus name, time until departure
    return NULL;
}
void free_departure_list(DepartureList *list) {
    if (list) {
        free(list->items);
        free(list);
    }
}
