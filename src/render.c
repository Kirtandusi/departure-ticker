#include "../include/render.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

/**
 * Console printing
 */
void print_output(DepartureList *list) {
    if (!list || list->count == 0) {
        printf("No upcoming departures.\n");
        return;
    }

    // Set timezone once
    setenv("TZ", "America/Chicago", 1);
    tzset();

    time_t now = time(NULL);

    printf("EASTBOUND\n");

    for (size_t i = 0; i < list->count; i++) {
        BusDeparture *d = &list->items[i];
        time_t arrival = d->arrival_unix_time;

        if (arrival == 0) {
            printf("%-5s ...\n", d->route_name);
            continue;
        }

        long mins = (arrival - now + 59) / 60; // round up

        if (mins < 0 || mins > 12 * 60) {
            printf("%-5s ...\n", d->route_name);
        } else {
            struct tm tm;
            localtime_r(&arrival, &tm);
            printf("%-5s %2ld min (%02d:%02d)\n",
                   d->route_name, mins,
                   tm.tm_hour, tm.tm_min);
        }
    }
}
/**
 * RGB Matrix display rendering. Should look identical to print_output, but have color. 
 */
void render_display(DepartureList *list) {
    // Placeholder for future display rendering logic
    if (!list) return;
    
}
