#define _POSIX_C_SOURCE 200809L
#include "../include/render.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

/*
 * hzeller/rpi-rgb-led-matrix  C API
 * https://github.com/hzeller/rpi-rgb-led-matrix/blob/master/include/led-matrix-c.h
 *
 * Build your project against the static lib:
 *     gcc … -I<matrix>/include  -L<matrix>/lib -lrgbmatrix -lrt -lm -lpthread
 */
#include "led-matrix-c.h"   /* ships with rpi-rgb-led-matrix */

/* ---------------------------------------------------------------------------
 * Panel / layout constants
 * --------------------------------------------------------------------------- */
#define COLS   64
#define ROWS   32

/* Font metrics  (5-wide, 8-tall glyph table below) */
#define FONT_W    5
#define FONT_H    8
#define CHAR_GAP  1          /* pixels between glyphs horizontally */
#define LINE_GAP  2          /* pixels between text rows            */
#define LINE_H    (FONT_H + LINE_GAP)   /* 10 px per line           */

/* ---------------------------------------------------------------------------
 * Colour palette
 * --------------------------------------------------------------------------- */
typedef struct { uint8_t r, g, b; } RGB;

static const RGB C_WHITE  = { 255, 255, 255 };
static const RGB C_GREEN  = {   0, 200,   0 };   /* ≤ 5 min            */
static const RGB C_YELLOW = { 220, 200,   0 };   /* 6–12 min           */
static const RGB C_GRAY   = { 100, 100, 100 };   /* unknown / "..."    */
static const RGB C_DIM    = {  40,  40,  40 };   /* header bg strip    */

/* ---------------------------------------------------------------------------
 * Minimal 5×5 ASCII font  (0x20 – 0x7E)
 *
 * Each entry is 5 bytes = 5 rows of the glyph.  Rows 5-7 are implicit zeros
 * (the FONT_H padding handled in fb_draw_char).
 * Bit layout per byte:  bit4=col0 … bit0=col4  (MSB-first, left-to-right).
 * --------------------------------------------------------------------------- */
/* clang-format off */
static const uint8_t font[][5] = {
    /* 0x20 */ {0x00,0x00,0x00,0x00,0x00}, /* ' '  */
    /* 0x21 */ {0x08,0x08,0x08,0x00,0x08}, /* '!'  */
    /* 0x22 */ {0x14,0x14,0x00,0x00,0x00}, /* '"'  */
    /* 0x23 */ {0x14,0x3E,0x14,0x3E,0x14}, /* '#'  */
    /* 0x24 */ {0x08,0x1E,0x38,0x1E,0x08}, /* '$'  */
    /* 0x25 */ {0x30,0x34,0x18,0x2C,0x06}, /* '%'  */
    /* 0x26 */ {0x10,0x28,0x10,0x2A,0x10}, /* '&'  */
    /* 0x27 */ {0x08,0x08,0x00,0x00,0x00}, /* '''  */
    /* 0x28 */ {0x04,0x08,0x10,0x08,0x04}, /* '('  */
    /* 0x29 */ {0x10,0x08,0x04,0x08,0x10}, /* ')'  */
    /* 0x2A */ {0x00,0x0A,0x04,0x0A,0x00}, /* '*'  */
    /* 0x2B */ {0x00,0x08,0x1C,0x08,0x00}, /* '+'  */
    /* 0x2C */ {0x00,0x00,0x00,0x08,0x10}, /* ','  */
    /* 0x2D */ {0x00,0x00,0x1C,0x00,0x00}, /* '-'  */
    /* 0x2E */ {0x00,0x00,0x00,0x00,0x08}, /* '.'  */
    /* 0x2F */ {0x02,0x04,0x08,0x10,0x20}, /* '/'  */
    /* 0x30 */ {0x1C,0x22,0x2A,0x26,0x1C}, /* '0'  */
    /* 0x31 */ {0x08,0x18,0x08,0x08,0x1C}, /* '1'  */
    /* 0x32 */ {0x1C,0x02,0x1C,0x20,0x3E}, /* '2'  */
    /* 0x33 */ {0x1C,0x02,0x1C,0x02,0x1C}, /* '3'  */
    /* 0x34 */ {0x0A,0x0A,0x1E,0x02,0x02}, /* '4'  */
    /* 0x35 */ {0x3E,0x20,0x3C,0x02,0x1C}, /* '5'  */
    /* 0x36 */ {0x1C,0x20,0x3C,0x22,0x1C}, /* '6'  */
    /* 0x37 */ {0x3E,0x02,0x04,0x08,0x08}, /* '7'  */
    /* 0x38 */ {0x1C,0x22,0x1C,0x22,0x1C}, /* '8'  */
    /* 0x39 */ {0x1C,0x22,0x1E,0x02,0x1C}, /* '9'  */
    /* 0x3A */ {0x00,0x08,0x00,0x08,0x00}, /* ':'  */
    /* 0x3B */ {0x00,0x08,0x00,0x08,0x10}, /* ';'  */
    /* 0x3C */ {0x04,0x08,0x10,0x08,0x04}, /* '<'  */
    /* 0x3D */ {0x00,0x1C,0x00,0x1C,0x00}, /* '='  */
    /* 0x3E */ {0x10,0x08,0x04,0x08,0x10}, /* '>'  */
    /* 0x3F */ {0x1C,0x02,0x0C,0x00,0x08}, /* '?'  */
    /* 0x40 */ {0x1C,0x22,0x2E,0x20,0x1C}, /* '@'  */
    /* 0x41 */ {0x08,0x14,0x22,0x3E,0x22}, /* 'A'  */
    /* 0x42 */ {0x3C,0x22,0x3C,0x22,0x3C}, /* 'B'  */
    /* 0x43 */ {0x1C,0x22,0x20,0x22,0x1C}, /* 'C'  */
    /* 0x44 */ {0x38,0x24,0x22,0x24,0x38}, /* 'D'  */
    /* 0x45 */ {0x3E,0x20,0x3C,0x20,0x3E}, /* 'E'  */
    /* 0x46 */ {0x3E,0x20,0x3C,0x20,0x20}, /* 'F'  */
    /* 0x47 */ {0x1C,0x22,0x20,0x26,0x1C}, /* 'G'  */
    /* 0x48 */ {0x22,0x22,0x3E,0x22,0x22}, /* 'H'  */
    /* 0x49 */ {0x1C,0x08,0x08,0x08,0x1C}, /* 'I'  */
    /* 0x4A */ {0x0E,0x04,0x04,0x24,0x18}, /* 'J'  */
    /* 0x4B */ {0x22,0x24,0x28,0x24,0x22}, /* 'K'  */
    /* 0x4C */ {0x20,0x20,0x20,0x20,0x3E}, /* 'L'  */
    /* 0x4D */ {0x22,0x36,0x2A,0x22,0x22}, /* 'M'  */
    /* 0x4E */ {0x22,0x32,0x2A,0x26,0x22}, /* 'N'  */
    /* 0x4F */ {0x1C,0x22,0x22,0x22,0x1C}, /* 'O'  */
    /* 0x50 */ {0x3C,0x22,0x3C,0x20,0x20}, /* 'P'  */
    /* 0x51 */ {0x1C,0x22,0x22,0x2A,0x1C}, /* 'Q'  */
    /* 0x52 */ {0x3C,0x22,0x3C,0x28,0x24}, /* 'R'  */
    /* 0x53 */ {0x1C,0x22,0x1C,0x02,0x1C}, /* 'S'  */
    /* 0x54 */ {0x1C,0x08,0x08,0x08,0x08}, /* 'T'  */
    /* 0x55 */ {0x22,0x22,0x22,0x22,0x1C}, /* 'U'  */
    /* 0x56 */ {0x22,0x22,0x22,0x14,0x08}, /* 'V'  */
    /* 0x57 */ {0x22,0x22,0x2A,0x36,0x22}, /* 'W'  */
    /* 0x58 */ {0x22,0x14,0x08,0x14,0x22}, /* 'X'  */
    /* 0x59 */ {0x22,0x14,0x08,0x08,0x08}, /* 'Y'  */
    /* 0x5A */ {0x3E,0x04,0x08,0x10,0x3E}, /* 'Z'  */
    /* 0x5B */ {0x1C,0x10,0x10,0x10,0x1C}, /* '['  */
    /* 0x5C */ {0x20,0x10,0x08,0x04,0x02}, /* '\'  */
    /* 0x5D */ {0x1C,0x04,0x04,0x04,0x1C}, /* ']'  */
    /* 0x5E */ {0x08,0x14,0x22,0x00,0x00}, /* '^'  */
    /* 0x5F */ {0x00,0x00,0x00,0x00,0x3E}, /* '_'  */
    /* 0x60 */ {0x10,0x08,0x04,0x00,0x00}, /* '`'  */
    /* 0x61 */ {0x00,0x1C,0x02,0x1E,0x1E}, /* 'a'  */
    /* 0x62 */ {0x20,0x3C,0x22,0x22,0x3C}, /* 'b'  */
    /* 0x63 */ {0x00,0x1C,0x20,0x20,0x1C}, /* 'c'  */
    /* 0x64 */ {0x02,0x1E,0x22,0x22,0x1E}, /* 'd'  */
    /* 0x65 */ {0x00,0x1C,0x22,0x3C,0x20}, /* 'e'  */
    /* 0x66 */ {0x0C,0x10,0x1C,0x10,0x10}, /* 'f'  */
    /* 0x67 */ {0x00,0x1E,0x22,0x1E,0x02}, /* 'g'  */
    /* 0x68 */ {0x20,0x3C,0x22,0x22,0x22}, /* 'h'  */
    /* 0x69 */ {0x08,0x00,0x18,0x08,0x1C}, /* 'i'  */
    /* 0x6A */ {0x04,0x00,0x0C,0x04,0x24}, /* 'j'  */
    /* 0x6B */ {0x20,0x24,0x28,0x30,0x28}, /* 'k'  */
    /* 0x6C */ {0x18,0x08,0x08,0x08,0x1C}, /* 'l'  */
    /* 0x6D */ {0x00,0x34,0x2A,0x2A,0x22}, /* 'm'  */
    /* 0x6E */ {0x00,0x2C,0x12,0x12,0x12}, /* 'n'  */
    /* 0x6F */ {0x00,0x1C,0x22,0x22,0x1C}, /* 'o'  */
    /* 0x70 */ {0x00,0x3C,0x22,0x3C,0x20}, /* 'p'  */
    /* 0x71 */ {0x00,0x1E,0x22,0x1E,0x02}, /* 'q'  */
    /* 0x72 */ {0x00,0x2C,0x12,0x10,0x10}, /* 'r'  */
    /* 0x73 */ {0x00,0x1E,0x20,0x1C,0x1E}, /* 's'  */
    /* 0x74 */ {0x10,0x1C,0x10,0x10,0x0C}, /* 't'  */
    /* 0x75 */ {0x00,0x22,0x22,0x22,0x1E}, /* 'u'  */
    /* 0x76 */ {0x00,0x22,0x22,0x14,0x08}, /* 'v'  */
    /* 0x77 */ {0x00,0x22,0x2A,0x2A,0x14}, /* 'w'  */
    /* 0x78 */ {0x00,0x22,0x14,0x08,0x22}, /* 'x'  */
    /* 0x79 */ {0x00,0x22,0x1E,0x02,0x1C}, /* 'y'  */
    /* 0x7A */ {0x00,0x3E,0x04,0x10,0x3E}, /* 'z'  */
    /* 0x7B */ {0x0C,0x08,0x38,0x08,0x0C}, /* '{'  */
    /* 0x7C */ {0x08,0x08,0x08,0x08,0x08}, /* '|'  */
    /* 0x7D */ {0x30,0x08,0x0E,0x08,0x30}, /* '}'  */
    /* 0x7E */ {0x10,0x2A,0x04,0x00,0x00}, /* '~'  */
};
/* clang-format on */

/* ---------------------------------------------------------------------------
 * Module-level state  (the hzeller handles / canvases)
 * --------------------------------------------------------------------------- */
static struct rgb_led_matrix *g_matrix   = NULL;
static struct led_canvas     *g_canvas   = NULL;   /* back buffer we draw on */

/* ---------------------------------------------------------------------------
 * matrix_init() – library bootstrap
 *
 * Flags chosen for this specific panel:
 *   rows          = 32
 *   cols          = 64
 *   chain_length  = 1
 *   parallel      = 1
 *   scan_mode     = 0   (progressive – default for 1:16 panels)
 *   gpio_mapping  = "regular"
 *   no_hardware_pulse = 1   (required when audio is off; see Pi config notes)
 *   slowdown_gpio = 2       (common sweet-spot; drop to 1 if you see tearing)
 *   brightness    = 60      (percent; 5mm-pitch indoors is bright enough here)
 * --------------------------------------------------------------------------- */
int matrix_init(void)
{
    struct rgb_matrix_config cfg;
    rgb_matrix_config_init(&cfg);          /* zero + sane defaults */

    cfg.rows            = ROWS;
    cfg.cols            = COLS;
    cfg.chain_length    = 1;
    cfg.parallel        = 1;
    cfg.scan_mode       = 0;               /* progressive           */
    cfg.pwm_bits        = 11;              /* max colour depth      */
    cfg.pwm_lsb_nanoseconds = 700;         /* good balance speed/flicker */
    cfg.slowdown_gpio   = 2;

    /* rgb_matrix_create_from_flags wants (config, argc, argv, errors).
     * We pass a minimal argv with just the flags we need; the library
     * parses them the same way it would from the command line.        */
    const char *argv[] = {
        "bus-display",              /* argv[0] – ignored by the lib */
        "--led-no-hardware-pulse",
        NULL
    };
    int argc = 2;   /* don't count the NULL sentinel */

    char errors[256];
    g_matrix = rgb_matrix_create_from_flags(&cfg, argc, argv, errors);
    if (!g_matrix) {
        fprintf(stderr, "matrix_init: rgb_matrix_create failed: %s\n", errors);
        return -1;
    }

    rgb_matrix_set_brightness(g_matrix, 60);

    /* Allocate the first (back) canvas; a second one is created implicitly
     * on the first SwapCanvas call for double buffering.              */
    g_canvas = rgb_matrix_create_canvas(g_matrix);
    if (!g_canvas) {
        fprintf(stderr, "matrix_init: canvas allocation failed\n");
        rgb_matrix_delete(g_matrix);
        g_matrix = NULL;
        return -1;
    }

    return 0;
}

/* ---------------------------------------------------------------------------
 * matrix_cleanup()
 * --------------------------------------------------------------------------- */
void matrix_cleanup(void)
{
    if (g_matrix) {
        rgb_matrix_delete(g_matrix);   /* also frees all canvases */
        g_matrix = NULL;
        g_canvas = NULL;
    }
}

/* ---------------------------------------------------------------------------
 * Font / drawing primitives  (operate on the hzeller canvas directly —
 *   no intermediate framebuffer needed; the library handles double-buffering)
 * --------------------------------------------------------------------------- */

/* Draw one character; returns x just past the glyph. */
static int draw_char(struct led_canvas *canvas,
                     int px, int py, char ch, RGB c)
{
    if (ch < 0x20 || ch > 0x7E) ch = '?';
    const uint8_t *g = font[(unsigned char)ch - 0x20];

    for (int row = 0; row < FONT_H; row++) {
        uint8_t byte = (row < 5) ? g[row] : 0;   /* rows 5-7 = blank pad */
        for (int col = 0; col < FONT_W; col++) {
            if (byte & (0x10 >> col))
                led_canvas_set_pixel(canvas, px + col, py + row,
                                     c.r, c.g, c.b);
        }
    }
    return px + FONT_W + CHAR_GAP;
}

/* Draw a string; returns x just past the last glyph. */
static int draw_str(struct led_canvas *canvas,
                    int px, int py, const char *s, RGB c)
{
    while (*s)
        px = draw_char(canvas, px, py, *s++, c);
    return px;
}

/* Right-align helper: returns the x that places the string's right edge
 * at `right_edge`.                                                         */
static int right_x(const char *s, int right_edge)
{
    int w = (int)strlen(s) * (FONT_W + CHAR_GAP) - CHAR_GAP;
    return right_edge - w;
}

/* ---------------------------------------------------------------------------
 * print_output()  – console, unchanged from original
 * --------------------------------------------------------------------------- */
void print_output(DepartureList *list)
{
    if (!list || list->count == 0) {
        printf("No upcoming departures.\n");
        return;
    }

    setenv("TZ", "America/Chicago", 1);
    tzset();
    time_t now = time(NULL);

    printf("EASTBOUND\n");
    for (size_t i = 0; i < list->count; i++) {
        BusDeparture *d      = &list->items[i];
        time_t        arrival = (time_t)d->arrival_unix_time;
        if (arrival == 0) {
            printf("%-5s ...\n", d->route_name);
            continue;
        }
        long mins = (arrival - now + 59) / 60;
        if (mins < 0 || mins > 12 * 60) {
            printf("%-5s ...\n", d->route_name);
        } else {
            struct tm tm;
            localtime_r(&arrival, &tm);
            printf("%-5s %2ld min (%02d:%02d)\n",
                   d->route_name, mins, tm.tm_hour, tm.tm_min);
        }
    }
}

/* ---------------------------------------------------------------------------
 * render_display()  – RGB matrix
 *
 * Layout (64 × 32, LINE_H = 10 px  →  3 data rows fit under the header):
 *
 *   y=0   [████ EASTBOUND ████████████████████████████████]  ← dim bg strip
 *   y=10  [ Route   3 min                         (14:07) ]  ← green
 *   y=20  [ Route   8 min                         (14:12) ]  ← yellow
 *   y=30  [ Route   ...                                   ]  ← gray
 *
 * Colour rules  (mirrors print_output semantics):
 *     ≤  5 min  →  GREEN     route name always WHITE
 *      6–12 min →  YELLOW    clock  "(HH:MM)" always GRAY
 *     unknown   →  GRAY  "..."
 * --------------------------------------------------------------------------- */
void render_display(DepartureList *list)
{
    if (!list || !g_matrix || !g_canvas) return;

    /* ── clear back buffer ──────────────────────────────────────────────── */
    led_canvas_clear(g_canvas);

    /* ── header ─────────────────────────────────────────────────────────── */
    for (int x = 0; x < COLS; x++)
        led_canvas_set_pixel(g_canvas, x, 0, C_DIM.r, C_DIM.g, C_DIM.b);
    draw_str(g_canvas, 2, 0, "EASTBOUND", C_WHITE);

    if (list->count == 0) {
        draw_str(g_canvas, 2, LINE_H, "No departures", C_GRAY);
        /* swap: back → front; the old front becomes the new back buffer */
        g_canvas = led_canvas_swap(g_matrix, g_canvas);
        return;
    }

    /* ── timezone ───────────────────────────────────────────────────────── */
    setenv("TZ", "America/Chicago", 1);
    tzset();
    time_t now = time(NULL);

    /* ── departure rows ─────────────────────────────────────────────────── */
    size_t max_rows = (ROWS - LINE_H) / LINE_H;   /* 2 on 32-row; 3 if you add a row */
    size_t count    = list->count < max_rows ? list->count : max_rows;

    for (size_t i = 0; i < count; i++) {
        BusDeparture *d      = &list->items[i];
        int           y      = (int)(LINE_H + i * LINE_H);
        time_t        arrival = (time_t)d->arrival_unix_time;

        /* route name – left-aligned, white */
        int x_end = draw_str(g_canvas, 2, y, d->route_name, C_WHITE);

        /* unknown / zero arrival */
        if (arrival == 0) {
            draw_str(g_canvas, x_end + 2, y, "...", C_GRAY);
            continue;
        }

        long mins = (arrival - now + 59) / 60;   /* round-up, same as print */

        if (mins < 0 || mins > 12 * 60) {
            draw_str(g_canvas, x_end + 2, y, "...", C_GRAY);
            continue;
        }

        RGB time_col = (mins <= 5) ? C_GREEN : C_YELLOW;

        char buf_min[12], buf_time[8];
        struct tm tm;
        localtime_r(&arrival, &tm);
        snprintf(buf_min,  sizeof(buf_min),  "%ld min", mins);
        snprintf(buf_time, sizeof(buf_time), "(%02d:%02d)",
                 tm.tm_hour, tm.tm_min);

        /* clock right-aligned, 2 px from the right edge */
        draw_str(g_canvas, right_x(buf_time, COLS - 2), y, buf_time, C_GRAY);

        /* "XX min" just after the route name */
        draw_str(g_canvas, x_end + 2, y, buf_min, time_col);
    }

    /* ── atomic swap – panel shows new frame instantly ─────────────────── */
    g_canvas = led_canvas_swap(g_matrix, g_canvas);
}