#include "parser.h"
#include <stdio.h>
typedef struct {
    char route_name[16];   
    long departure_epoch;  
    int departure_minutes; 
} BusDeparture;

typedef struct {
    BusDeparture *items;
    int count;
} DepartureList;

DepartureList *parse_json(const char *json, char *stop) {
    if (!json) {
        printf("No JSON to parse\n");
        return;
    }

    // TODO: implement parsing logic

    //return bus name, time until departure
    return NULL;
}
