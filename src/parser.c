#include "parser.h"
#include <stdio.h>

DepartureList *parse_json(char *json, char *stop) {
    if (!json) {
        printf("No JSON to parse\n");
        return NULL;
    }
    DepartureList *list = malloc(sizeof(DepartureList));
    if (!list) {
        printf("Memory allocation failed\n");
        return NULL;
    }
    //stubbing
    list->items = NULL;
    list->count = 0;
    //return bus name, time until departure
    for (int i = 0; json[i] != '\0'; i++) {
        
    }
    return list;
}
void free_departure_list(DepartureList *list) {
    if (list) {
        free(list->items);
        free(list);
    }
}
