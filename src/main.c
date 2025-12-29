#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "../include/networking.h"
#include "../include/parser.h"
#include "../include/render.h"

int main() {
    printf("Fetching bus data...\n");

    // Stops and corresponding route letters
    char *stop_ids[] = {"133", "857", "533", "887"};
    char *route_letters[] = {"C", "D", "E", "A"};
    size_t n_stops = sizeof(stop_ids) / sizeof(stop_ids[0]);

    while (1) {
        size_t pb_size = 0;
        char *pb = get_data(&pb_size);

        if (!pb || pb_size == 0) {
            fprintf(stderr, "Failed to get data\n");
            sleep(15);
            continue;
        }

        // Parse feed for these stops
        DepartureList *list = parse_pb_to_list(pb, pb_size, stop_ids, route_letters, n_stops);
        free(pb);

        if (!list) {
            fprintf(stderr, "Failed to parse data\n");
            sleep(15);
            continue;
        }

        // Render the departures
        render_display(list);
        printf("Parsed %zu departures\n\n", list->count);

        free_departure_list(list);

        sleep(15); // ~5760 requests/day
    }

    return 0;
}
