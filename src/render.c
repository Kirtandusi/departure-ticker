#include "../include/render.h"
#include "../include/parser.h"
#include <stdio.h>

void render_display(DepartureList *list) {
    if (!list || list->count == 0) {
        printf("No departures.\n");
        return;
    }

    printf("Departures (%zu):\n", list->count);
    for (size_t i = 0; i < list->count; i++) {
        printf("Route %s arriving at %ld\n",
               list->items[i].route_name,
               list->items[i].departure_minutes);
    }
}

