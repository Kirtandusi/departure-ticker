#include "../include/render.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

static void print_time_central(time_t t) {
    struct tm tm;
    setenv("TZ", "America/Chicago", 1);
    tzset();
    localtime_r(&t, &tm);
    printf("%02d:%02d", tm.tm_hour, tm.tm_min);
}

void render_display(DepartureList *list) {
    if (!list || list->count == 0) {
        printf("No upcoming departures.\n");
        return;
    }

    time_t now = time(NULL);

    printf("EASTBOUND\n");
    for (size_t i = 0; i < list->count; i++) {
        long mins = (list->items[i].arrival_unix_time - now) / 60;

        // If no departure or >12 hours ahead
        if (list->items[i].arrival_unix_time == 0 || mins > 12*60)
            printf("%-5s ...\n", list->items[i].route_name);
        else
            printf("%-5s %2ld min (", list->items[i].route_name, mins),
            print_time_central(list->items[i].arrival_unix_time),
            printf(")\n");
    }
}
