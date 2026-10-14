#define _POSIX_C_SOURCE 200809L
#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

long departure_minutes(const BusDeparture *departure, time_t now) {
    if (!departure || !departure->arrival_unix_time) return -1;
    int64_t seconds = departure->arrival_unix_time - (int64_t)now;
    if (seconds < -59 || seconds > 12 * 60 * 60) return -1;
    return seconds <= 0 ? 0 : (long)((seconds + 59) / 60);
}

void print_output(const DepartureList *list, time_t now) {
    puts("DEPARTURES (America/Chicago)");
    if (!list || !list->count) { puts("No upcoming departures."); return; }
    for (size_t i = 0; i < list->count; ++i) {
        const BusDeparture *departure = &list->items[i];
        long minutes = departure_minutes(departure, now);
        if (minutes < 0) {
            printf("Stop %-5s  No upcoming departures\n", departure->stop_id);
        } else {
            time_t arrival = (time_t)departure->arrival_unix_time;
            struct tm local;
            if (!localtime_r(&arrival, &local)) continue;
            printf("Stop %-5s  %-7s %3ld min (%02d:%02d)\n", departure->stop_id,
                   departure->route_name, minutes, local.tm_hour, local.tm_min);
        }
    }
    fflush(stdout);
}

#ifdef ENABLE_MATRIX
#include <led-matrix-c.h>
#ifndef DEFAULT_MATRIX_FONT
#define DEFAULT_MATRIX_FONT "/usr/local/share/departure-ticker/5x7.bdf"
#endif
static struct RGBLedMatrix *matrix;
static struct LedCanvas *canvas;
static struct LedFont *font;
static int width, height, row_height, baseline, cell_width;

int render_init(int *argc, char ***argv, int enabled) {
    if (!enabled) return 0;
    const char *font_path = getenv("MATRIX_FONT");
    font = load_font(font_path && *font_path ? font_path : DEFAULT_MATRIX_FONT);
    if (!font) { fprintf(stderr, "Cannot load MATRIX_FONT (BDF font).\n"); return -1; }
    struct RGBLedMatrixOptions options = {0};
    options.rows = 32;
    options.cols = 64;
    options.chain_length = 1;
    options.parallel = 1;
    options.brightness = 50;
    matrix = led_matrix_create_from_options(&options, argc, argv);
    if (!matrix) { render_shutdown(); return -1; }
    canvas = led_matrix_create_offscreen_canvas(matrix);
    if (!canvas) { render_shutdown(); return -1; }
    led_canvas_get_size(canvas, &width, &height);
    row_height = height_font(font) + 1;
    baseline = baseline_font(font);
    cell_width = character_width_font(font, 'M');
    if (cell_width <= 0 || row_height <= 0 || height < 2 * row_height ||
        width < 6 * cell_width) {
        fprintf(stderr, "Panel is too small for the selected font.\n");
        render_shutdown();
        return -1;
    }
    return 0;
}

void render_display(const DepartureList *list, time_t now, unsigned page_tick, int demo) {
    if (!matrix || !list) return;
    led_canvas_clear(canvas);
    size_t per_page = (size_t)(height / row_height - 1);
    size_t pages = (list->count + per_page - 1) / per_page;
    size_t page = pages ? (page_tick / 4) % pages : 0;
    char heading[64];
    snprintf(heading, sizeof(heading), "%s %zu/%zu", demo ? "DEMO" : "BUS",
             page + 1, pages ? pages : 1);
    draw_text(canvas, font, 0, baseline, 40, 160, 255, heading, 0);
    for (size_t row = 0; row < per_page; ++row) {
        size_t index = page * per_page + row;
        if (index >= list->count) break;
        const BusDeparture *departure = &list->items[index];
        long minutes = departure_minutes(departure, now);
        char remaining[32];
        if (minutes < 0) snprintf(remaining, sizeof(remaining), "--");
        else if (!minutes) snprintf(remaining, sizeof(remaining), "DUE");
        else snprintf(remaining, sizeof(remaining), "%ldm", minutes);
        int right = width - (int)strlen(remaining) * cell_width;
        int label_chars = right / cell_width - 1;
        if (label_chars > ROUTE_NAME_SIZE - 1) label_chars = ROUTE_NAME_SIZE - 1;
        char label[ROUTE_NAME_SIZE];
        /* Alternate route and stop ID so repeated routes remain identifiable. */
        const char *name = minutes < 0 || (page_tick % 4 >= 2)
                         ? departure->stop_id : departure->route_name;
        snprintf(label, sizeof(label), "%.*s", label_chars, name);
        int y = baseline + (int)(row + 1) * row_height;
        draw_text(canvas, font, 0, y, 200, 200, 200, label, 0);
        draw_text(canvas, font, right, y, minutes >= 0 && minutes <= 2 ? 255 : 40,
                  minutes < 0 ? 100 : 220, 40, remaining, 0);
    }
    canvas = led_matrix_swap_on_vsync(matrix, canvas);
}

void render_error(void) {
    if (!matrix) return;
    led_canvas_clear(canvas);
    draw_text(canvas, font, 0, baseline, 255, 80, 40, "OFFLINE", 0);
    draw_text(canvas, font, 0, baseline + row_height, 200, 200, 200, "RETRYING", 0);
    canvas = led_matrix_swap_on_vsync(matrix, canvas);
}

void render_shutdown(void) {
    if (matrix) led_matrix_delete(matrix);
    if (font) delete_font(font);
    matrix = NULL;
    font = NULL;
    canvas = NULL;
}
#else
int render_init(int *argc, char ***argv, int enabled) {
    (void)argc;
    (void)argv;
    if (enabled) fprintf(stderr, "Matrix support is not built; use make MATRIX=1.\n");
    return enabled ? -1 : 0;
}
void render_display(const DepartureList *list, time_t now, unsigned page_tick, int demo) {
    (void)list; (void)now; (void)page_tick; (void)demo;
}
void render_error(void) {}
void render_shutdown(void) {}
#endif
