#include <stdio.h>
#include <stdlib.h>
#include "networking.h"
#include "parser.h"
#include "render.h"
#include <unistd.h>
#include <time.h>
int main() {
    printf("Fetching bus data...\n");
    while (1) {
        char *pb = get_data();
        if (pb) {
            DepartureList *list = parse_pb_to_list(pb, "133"); //N Broom at W Dayton ID
            render_display(list);
            free(pb);
            free_departure_list(list);
        } else {
            fprintf(stderr, "Failed to get data\n");
        }
        sleep(15); // max cap of 10,000 requests per day. This reduces requests to 5760 per day.
    }
    return 0;
}
