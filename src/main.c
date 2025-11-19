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
        char *json = get_data();
        if (json) {
            DepartureList *list = parse_json(json, "N Broom at W Dayton");
            render_display(list);
            free(json);
            free_departure_list(list);
        } else {
            fprintf(stderr, "Failed to get data\n");
        }
        sleep(1); // Wait for 60 seconds before fetching data again
    }
    return 0;
}
