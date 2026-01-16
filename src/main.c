#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "../include/networking.h"
#include "../include/parser.h"
#include "../include/render.h"

int main() {
    // Define your stops and their letters
    char *stop_ids[] = {"887", "133", "857", "533"};
    char *route_letters[] = {"A", "C", "D", "E"};
    size_t n_stops = 4;

    printf("Fetching bus data...\n");

    while (1) {
        size_t pb_size = 0;
        char *pb = get_data(&pb_size);

        if (!pb || pb_size == 0) {
            fprintf(stderr, "Failed to get data\n");
            sleep(15);
            continue;
        }

        DepartureList *list = parse_pb_to_list(pb, pb_size, stop_ids, route_letters, n_stops);
        free(pb);

        if (!list) {
            fprintf(stderr, "Failed to parse data\n");
            sleep(15);
            continue;
        }

        render_display(list);
        printf("Parsed %zu departures\n\n", list->count);

        free_departure_list(list);

        sleep(15); // ~5760 requests/day
    }

    return 0;
}
