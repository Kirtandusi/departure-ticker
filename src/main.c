#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include "../include/networking.h"
#include "../include/parser.h"
#include "../include/render.h"

//shut down on ctrl+c
static volatile sig_atomic_t g_running = 1;

static void sig_handler(int sig)
{
    (void)sig;
    g_running = 0;
}
void load_dotenv(const char *path)
{
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
        char *nl  = strchr(val, '\n');
        if (nl) *nl = 0;
        setenv(key, val, 1);
    }
    fclose(f);
}


int main(void)
{
    /* ── signal handlers ──────────────────────────────────────────────── */
    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);

    /* ── env ──────────────────────────────────────────────────────────── */
    load_dotenv("src/.env");
    const char *API_KEY = getenv("API_KEY");
    if (!API_KEY) {
        fprintf(stderr, "API_KEY not set in environment\n");
        return 1;
    }

    /* ── matrix init (must happen before render_display) ─────────────── */
    if (matrix_init() != 0) {
        fprintf(stderr, "Failed to initialise LED matrix\n");
        return 1;
    }

    /* ── fetch loop ───────────────────────────────────────────────────── */
    char *stop_ids[] = {"0454", "0133", "0857", "10062"};
    int   n_stops    = 4;

    printf("Fetching bus data...\n");

    while (g_running) {
        char *url = build_url(API_KEY, stop_ids, n_stops);

        size_t json_size = 0;
        char  *json      = get_data(url, &json_size);
        free(url);

        if (!json || json_size == 0) {
            fprintf(stderr, "Failed to get data\n");
            sleep(15);
            continue;
        }

        DepartureList *list = parse_json_predictions(json, stop_ids, n_stops);
        if (!list) {
            fprintf(stderr, "Failed to parse data\n");
            free(json);
            sleep(15);
            continue;
        }

        print_output(list);
        render_display(list);

        free_departure_list(list);
        free(json);
        sleep(15);
    }

    //exit
    matrix_cleanup();
    printf("\nShutdown complete.\n");
    return 0;
}