#define _POSIX_C_SOURCE 200809L
#include "config.h"
#include "networking.h"
#include "parser.h"
#include "render.h"
#include <curl/curl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t stopping;
static void stop(int signal_number) { (void)signal_number; stopping = 1; }

static void usage(const char *program) {
    printf("Usage: %s [--once] [--demo] [--matrix [--led-...]]\n"
           "  --once    Fetch/print once; exit nonzero on failure.\n"
           "  --demo    Use simulated departures, without an API key or network.\n"
           "  --matrix  Enable the RGB panel (requires a MATRIX=1 build).\n"
           "Environment: API_KEY, STOP_IDS, API_FEED, POLL_SECONDS, MATRIX_FONT.\n",
           program);
}

static DepartureList *demo_departures(char *const stops[], size_t count, time_t now) {
    DepartureList *list = calloc(1, sizeof(*list));
    if (!list) return NULL;
    list->items = calloc(count, sizeof(*list->items));
    if (!list->items) { free(list); return NULL; }
    list->count = count;
    for (size_t i = 0; i < count; ++i) {
        snprintf(list->items[i].stop_id, STOP_ID_SIZE, "%s", stops[i]);
        snprintf(list->items[i].route_name, ROUTE_NAME_SIZE, "%c", 'A' + (int)i);
        list->items[i].arrival_unix_time = (int64_t)now + (int64_t)(i + 1) * 180;
    }
    return list;
}

int main(int argc, char **argv) {
    int once = 0, demo = 0, matrix_enabled = 0, remaining = 1;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--help")) { usage(argv[0]); return 0; }
        if (!strcmp(argv[i], "--once")) once = 1;
        else if (!strcmp(argv[i], "--demo")) demo = 1;
        else if (!strcmp(argv[i], "--matrix")) matrix_enabled = 1;
        else if (!strncmp(argv[i], "--led-", 6)) argv[remaining++] = argv[i];
        else { fprintf(stderr, "Unknown option. Use --help.\n"); return 1; }
    }
    argc = remaining;
    argv[argc] = NULL;
    if (argc > 1 && !matrix_enabled) {
        fprintf(stderr, "LED options require --matrix.\n");
        return 1;
    }
    if (load_dotenv(".env") != 0 || load_dotenv("src/.env") != 0) {
        fprintf(stderr, "Cannot read dotenv configuration, or it contains an invalid line.\n");
        return 1;
    }
    const char *api_key = getenv("API_KEY");
    if (!demo && (!api_key || !*api_key)) {
        fprintf(stderr, "API_KEY is required; set it in the environment or .env.\n");
        return 1;
    }
    char storage[MAX_STOPS][STOP_ID_SIZE];
    char *stops[MAX_STOPS];
    size_t count;
    unsigned interval;
    const char *configured_stops = getenv("STOP_IDS");
    if (parse_stops(configured_stops ? configured_stops : "0454,0133,0857,10062",
                    storage, stops, &count) != 0 ||
        poll_interval(getenv("POLL_SECONDS"), &interval) != 0) {
        fprintf(stderr, "Use 1-10 distinct numeric STOP_IDS and POLL_SECONDS from 15 to 3600.\n");
        return 1;
    }
    if (init_transit_timezone() != 0 || curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        fprintf(stderr, "Cannot initialize timezone/networking.\n");
        return 1;
    }
    char *url = demo ? NULL : build_url(api_key, stops, count, getenv("API_FEED"));
    if (!demo && !url) {
        fprintf(stderr, "Cannot construct request; check configuration.\n");
        curl_global_cleanup();
        return 1;
    }
    if (render_init(&argc, &argv, matrix_enabled) != 0 || argc != 1) {
        fprintf(stderr, "Display initialization/options failed.\n");
        render_shutdown();
        free(url);
        curl_global_cleanup();
        return 1;
    }
    struct sigaction action = {0};
    action.sa_handler = stop;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    if (demo) puts("DEMO: simulated departures, not live bus times.");

    DepartureList *list = NULL;
    unsigned ticks = 0;
    int result = 0;
    while (!stopping) {
        if (ticks % interval == 0) {
            DepartureList *updated = NULL;
            if (demo) updated = demo_departures(stops, count, time(NULL));
            else {
                size_t json_size = 0;
                char *json = get_data(url, &json_size);
                if (json) updated = parse_json_predictions(json, stops, count, time(NULL));
                free(json);  /* Also released on every parse/API failure. */
            }
            free_departure_list(list);
            list = updated;
            if (!list) {
                fprintf(stderr, "Departures unavailable: network/API/response error; retrying.\n");
                render_error();
                result = 1;
            } else {
                print_output(list, time(NULL));
                result = 0;
            }
        }
        if (list) render_display(list, time(NULL), ticks, demo);
        if (once) break;
        sleep(1);
        ++ticks;
    }
    free_departure_list(list);
    render_shutdown();
    free(url);
    curl_global_cleanup();
    return stopping ? 0 : result;
}
