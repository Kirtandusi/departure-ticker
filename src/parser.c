#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

DepartureList *parse_json_to_list(char *json, char *stop) {
    
    DepartureList *list = malloc(sizeof(DepartureList));
    if (!list) return NULL;

    list->count = 0;
    list->items = malloc(sizeof(BusDeparture) * 1); // placeholder
    if (!list->items) {
        free(list);
        return NULL;
    }

    return list;
}

DepartureList *parse_pb_to_list(char *pb, char *stop) {
    if (!pb) {
        printf("No .pb to parse\n");
        return NULL;
    }
    char *json = NULL;


    return parse_json_to_list(json, stop);
    }
    

void free_departure_list(DepartureList *list) {
    if (list) {
        free(list->items);
        free(list);
    }
}