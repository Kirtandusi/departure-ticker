#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "../include/networking.h"
#include "../include/parser.h"
#include "../include/render.h"

void load_dotenv(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || strlen(line) < 3) continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;

        *eq = 0;
        char *key = line;
        char *val = eq + 1;

        // remove newline
        char *nl = strchr(val, '\n');
        if (nl) *nl = 0;

        setenv(key, val, 1);
    }
    fclose(f);
}
int main() {
    load_dotenv("src/.env");
    const char* API_KEY = getenv("API_KEY");
    if (!API_KEY) {
        fprintf(stderr, "API_KEY not set in environment\n");
        return 1;
    }
    char *stop_ids[] = {"0454", "0133", "0857", "10062"};
    //10062
    int n_stops = 4;

    printf("Fetching bus data...\n");

    while (1) {
        char *url = build_url(API_KEY, stop_ids, n_stops);

        size_t json_size = 0;
        char *json = get_data(url, &json_size);
        free(url);

        if (!json || json_size == 0) {
            fprintf(stderr, "Failed to get data\n");
            sleep(15);
            continue;
        }

        DepartureList *list = parse_json_predictions(json, stop_ids, n_stops);

        if (!list) {
            fprintf(stderr, "Failed to parse data\n");
            sleep(15);
            continue;
        }
        print_output(list);
        render_display(list);
        free_departure_list(list);
        free(json);
        sleep(15);
    }
}
