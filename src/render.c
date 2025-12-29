#include "../include/render.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

static void print_time_central(time_t t) {
    setenv("TZ", "America/Chicago", 1);
    tzset();

    struct tm tm;
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
        time_t arrival = list->items[i].arrival_unix_time;
        long mins = (arrival - now) / 60;

        if (mins < 0 || mins > 12 * 60) {
            printf("%-5s ...\n", list->items[i].route_name);
        } else {
            printf("%-5s %2ld min (", list->items[i].route_name, mins);
            print_time_central(arrival);
            printf(")\n");
        }
    }
}
