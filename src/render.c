#define _POSIX_C_SOURCE 200809L
#include "../include/render.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * Configuration — tweak these to match your physical setup
 * --------------------------------------------------------------------------- */
#ifndef MATRIX_WIDTH
#define MATRIX_WIDTH   64   /* total pixel columns (e.g. two 32-col panels) */
#endif
#ifndef MATRIX_HEIGHT
#define MATRIX_HEIGHT  32   /* total pixel rows */
#endif

/* Font metrics (matches the 5x8 table below) */
#define FONT_W  5
#define FONT_H  8
#define CHAR_GAP 1          /* 1-pixel space between characters */
#define LINE_GAP 2          /* 2-pixel gap between text rows */
#define LINE_HEIGHT (FONT_H + LINE_GAP)

/* ---------------------------------------------------------------------------
 * Colour palette  (R, G, B)
 * --------------------------------------------------------------------------- */
typedef struct { uint8_t r, g, b; } RGB;

static const RGB CLR_WHITE  = { 255, 255, 255 };
static const RGB CLR_GREEN  = {   0, 200,   0 };   /* ≤ 5 min */
static const RGB CLR_YELLOW = { 220, 200,   0 };   /* 6-12 min */
static const RGB CLR_GRAY   = { 100, 100, 100 };   /* unknown / "..." */
static const RGB CLR_DIM    = {  40,  40,  40 };   /* header background tint */

/* ---------------------------------------------------------------------------
 * Minimal 5×8 ASCII font (printable range 0x20–0x7E)
 * Each character is 5 bytes; bit 7 of each byte is the leftmost pixel,
 * rows are stored top-to-bottom.
 * --------------------------------------------------------------------------- */
/* clang-format off */
static const uint8_t font_5x8[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* 0x20 ' ' */
    {0x08,0x08,0x08,0x00,0x08}, /* 0x21 '!' */
    {0x14,0x14,0x00,0x00,0x00}, /* 0x22 '"' */
    {0x14,0x3E,0x14,0x3E,0x14}, /* 0x23 '#' */
    {0x08,0x1E,0x38,0x1E,0x08}, /* 0x24 '$' */
    {0x30,0x34,0x18,0x2C,0x06}, /* 0x25 '%' */
    {0x10,0x28,0x10,0x2A,0x10}, /* 0x26 '&' */
    {0x08,0x08,0x00,0x00,0x00}, /* 0x27 ''' */
    {0x04,0x08,0x10,0x08,0x04}, /* 0x28 '(' */
    {0x10,0x08,0x04,0x08,0x10}, /* 0x29 ')' */
    {0x00,0x0A,0x04,0x0A,0x00}, /* 0x2A '*' */
    {0x00,0x08,0x1C,0x08,0x00}, /* 0x2B '+' */
    {0x00,0x00,0x00,0x08,0x10}, /* 0x2C ',' */
    {0x00,0x00,0x1C,0x00,0x00}, /* 0x2D '-' */
    {0x00,0x00,0x00,0x00,0x08}, /* 0x2E '.' */
    {0x02,0x04,0x08,0x10,0x20}, /* 0x2F '/' */
    {0x1C,0x22,0x2A,0x26,0x1C}, /* 0x30 '0' */
    {0x08,0x18,0x08,0x08,0x1C}, /* 0x31 '1' */
    {0x1C,0x02,0x1C,0x20,0x3E}, /* 0x32 '2' */
    {0x1C,0x02,0x1C,0x02,0x1C}, /* 0x33 '3' */
    {0x0A,0x0A,0x1E,0x02,0x02}, /* 0x34 '4' */
    {0x3E,0x20,0x3C,0x02,0x1C}, /* 0x35 '5' */
    {0x1C,0x20,0x3C,0x22,0x1C}, /* 0x36 '6' */
    {0x3E,0x02,0x04,0x08,0x08}, /* 0x37 '7' */
    {0x1C,0x22,0x1C,0x22,0x1C}, /* 0x38 '8' */
    {0x1C,0x22,0x1E,0x02,0x1C}, /* 0x39 '9' */
    {0x00,0x08,0x00,0x08,0x00}, /* 0x3A ':' */
    {0x00,0x08,0x00,0x08,0x10}, /* 0x3B ';' */
    {0x04,0x08,0x10,0x08,0x04}, /* 0x3C '<' */
    {0x00,0x1C,0x00,0x1C,0x00}, /* 0x3D '=' */
    {0x10,0x08,0x04,0x08,0x10}, /* 0x3E '>' */
    {0x1C,0x02,0x0C,0x00,0x08}, /* 0x3F '?' */
    {0x1C,0x22,0x2E,0x20,0x1C}, /* 0x40 '@' */
    {0x08,0x14,0x22,0x3E,0x22}, /* 0x41 'A' */
    {0x3C,0x22,0x3C,0x22,0x3C}, /* 0x42 'B' */
    {0x1C,0x22,0x20,0x22,0x1C}, /* 0x43 'C' */
    {0x38,0x24,0x22,0x24,0x38}, /* 0x44 'D' */
    {0x3E,0x20,0x3C,0x20,0x3E}, /* 0x45 'E' */
    {0x3E,0x20,0x3C,0x20,0x20}, /* 0x46 'F' */
    {0x1C,0x22,0x20,0x26,0x1C}, /* 0x47 'G' */
    {0x22,0x22,0x3E,0x22,0x22}, /* 0x48 'H' */
    {0x1C,0x08,0x08,0x08,0x1C}, /* 0x49 'I' */
    {0x0E,0x04,0x04,0x24,0x18}, /* 0x4A 'J' */
    {0x22,0x24,0x28,0x24,0x22}, /* 0x4B 'K' */
    {0x20,0x20,0x20,0x20,0x3E}, /* 0x4C 'L' */
    {0x22,0x36,0x2A,0x22,0x22}, /* 0x4D 'M' */
    {0x22,0x32,0x2A,0x26,0x22}, /* 0x4E 'N' */
    {0x1C,0x22,0x22,0x22,0x1C}, /* 0x4F 'O' */
    {0x3C,0x22,0x3C,0x20,0x20}, /* 0x50 'P' */
    {0x1C,0x22,0x22,0x2A,0x1C}, /* 0x51 'Q' (note: tail pixel simplified) */
    {0x3C,0x22,0x3C,0x28,0x24}, /* 0x52 'R' */
    {0x1C,0x22,0x1C,0x02,0x1C}, /* 0x53 'S' */
    {0x1C,0x08,0x08,0x08,0x08}, /* 0x54 'T' */
    {0x22,0x22,0x22,0x22,0x1C}, /* 0x55 'U' */
    {0x22,0x22,0x22,0x14,0x08}, /* 0x56 'V' */
    {0x22,0x22,0x2A,0x36,0x22}, /* 0x57 'W' */
    {0x22,0x14,0x08,0x14,0x22}, /* 0x58 'X' */
    {0x22,0x14,0x08,0x08,0x08}, /* 0x59 'Y' */
    {0x3E,0x04,0x08,0x10,0x3E}, /* 0x5A 'Z' */
    {0x1C,0x10,0x10,0x10,0x1C}, /* 0x5B '[' */
    {0x20,0x10,0x08,0x04,0x02}, /* 0x5C '\' */
    {0x1C,0x04,0x04,0x04,0x1C}, /* 0x5D ']' */
    {0x08,0x14,0x22,0x00,0x00}, /* 0x5E '^' */
    {0x00,0x00,0x00,0x00,0x3E}, /* 0x5F '_' */
    {0x10,0x08,0x04,0x00,0x00}, /* 0x60 '`' */
    {0x00,0x1C,0x02,0x1E,0x1E}, /* 0x61 'a' */
    {0x20,0x3C,0x22,0x22,0x3C}, /* 0x62 'b' */
    {0x00,0x1C,0x20,0x20,0x1C}, /* 0x63 'c' */
    {0x02,0x1E,0x22,0x22,0x1E}, /* 0x64 'd' */
    {0x00,0x1C,0x22,0x3C,0x20}, /* 0x65 'e' (simplified) */
    {0x0C,0x10,0x1C,0x10,0x10}, /* 0x66 'f' */
    {0x00,0x1E,0x22,0x1E,0x02}, /* 0x67 'g' */
    {0x20,0x3C,0x22,0x22,0x22}, /* 0x68 'h' */
    {0x08,0x00,0x18,0x08,0x1C}, /* 0x69 'i' */
    {0x04,0x00,0x0C,0x04,0x24}, /* 0x6A 'j' (simplified) */
    {0x20,0x24,0x28,0x30,0x28}, /* 0x6B 'k' (simplified) */
    {0x18,0x08,0x08,0x08,0x1C}, /* 0x6C 'l' */
    {0x00,0x34,0x2A,0x2A,0x22}, /* 0x6D 'm' */
    {0x00,0x2C,0x12,0x12,0x12}, /* 0x6E 'n' (simplified) */
    {0x00,0x1C,0x22,0x22,0x1C}, /* 0x6F 'o' */
    {0x00,0x3C,0x22,0x3C,0x20}, /* 0x70 'p' */
    {0x00,0x1E,0x22,0x1E,0x02}, /* 0x71 'q' */
    {0x00,0x2C,0x12,0x10,0x10}, /* 0x72 'r' (simplified) */
    {0x00,0x1E,0x20,0x1C,0x1E}, /* 0x73 's' (simplified) */
    {0x10,0x1C,0x10,0x10,0x0C}, /* 0x74 't' */
    {0x00,0x22,0x22,0x22,0x1E}, /* 0x75 'u' */
    {0x00,0x22,0x22,0x14,0x08}, /* 0x76 'v' */
    {0x00,0x22,0x2A,0x2A,0x14}, /* 0x77 'w' */
    {0x00,0x22,0x14,0x08,0x22}, /* 0x78 'x' */
    {0x00,0x22,0x1E,0x02,0x1C}, /* 0x79 'y' */
    {0x00,0x3E,0x04,0x10,0x3E}, /* 0x7A 'z' */
    {0x0C,0x08,0x38,0x08,0x0C}, /* 0x7B '{' */
    {0x08,0x08,0x08,0x08,0x08}, /* 0x7C '|' */
    {0x30,0x08,0x0E,0x08,0x30}, /* 0x7D '}' */
    {0x10,0x2A,0x04,0x00,0x00}, /* 0x7E '~' */
};
/* clang-format on */

/* ---------------------------------------------------------------------------
 * Framebuffer
 * --------------------------------------------------------------------------- */
static RGB framebuffer[MATRIX_HEIGHT][MATRIX_WIDTH];

/* ---------------------------------------------------------------------------
 * Backend stubs — replace / wrap these with your actual matrix driver calls.
 *
 *   matrix_set_pixel(x, y, r, g, b)   – write one pixel
 *   matrix_flush()                     – push framebuffer → hardware
 *
 * For rpi-ws281x you would map (x,y) → LED index and call ws2811_led_set().
 * For Adafruit GFX you would call display.setPixel(x, y, colour).
 * --------------------------------------------------------------------------- */
void matrix_set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
void matrix_flush(void);

/* ---------------------------------------------------------------------------
 * Framebuffer helpers
 * --------------------------------------------------------------------------- */
static void fb_clear(void)
{
    memset(framebuffer, 0, sizeof(framebuffer));
}

static void fb_set(int x, int y, RGB c)
{
    if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT) return;
    framebuffer[y][x] = c;
}

/* Push entire framebuffer to the hardware in one go */
static void fb_flush(void)
{
    for (int y = 0; y < MATRIX_HEIGHT; y++)
        for (int x = 0; x < MATRIX_WIDTH; x++)
            matrix_set_pixel(x, y,
                             framebuffer[y][x].r,
                             framebuffer[y][x].g,
                             framebuffer[y][x].b);
    matrix_flush();
}

/* ---------------------------------------------------------------------------
 * Font renderer
 * --------------------------------------------------------------------------- */

/*
 * Draw a single character at pixel position (px, py).
 * Returns the x-coordinate just past the character (for chaining).
 */
static int fb_draw_char(int px, int py, char ch, RGB colour)
{
    if (ch < 0x20 || ch > 0x7E) ch = '?';
    const uint8_t *glyph = font_5x8[ch - 0x20];

    for (int row = 0; row < FONT_H; row++) {
        uint8_t byte = glyph[row < 5 ? row : 0]; /* font is 5 rows; pad */
        if (row < 5) byte = glyph[row];
        else         byte = 0;                    /* rows 5-7 are blank padding */
        for (int col = 0; col < FONT_W; col++) {
            /* bit 4 = leftmost column */
            if (byte & (0x10 >> col))
                fb_set(px + col, py + row, colour);
        }
    }
    return px + FONT_W + CHAR_GAP;
}

/*
 * Draw a null-terminated string starting at (px, py).
 * Returns the x-coordinate just past the last character drawn.
 */
static int fb_draw_string(int px, int py, const char *s, RGB colour)
{
    while (*s) {
        px = fb_draw_char(px, py, *s, colour);
        s++;
    }
    return px;
}

/* ---------------------------------------------------------------------------
 * Layout helpers
 * --------------------------------------------------------------------------- */

/*
 * Right-align a string so that its last character ends at column `right_edge`.
 * Returns the starting x for the string.
 */
static int calc_right_x(const char *s, int right_edge)
{
    int len = (int)strlen(s);
    int total_w = len * (FONT_W + CHAR_GAP) - CHAR_GAP; /* no trailing gap */
    return right_edge - total_w;
}

/* ---------------------------------------------------------------------------
 * Console printing  (unchanged)
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
        BusDeparture *d = &list->items[i];
        time_t        arrival = d->arrival_unix_time;
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
 * RGB Matrix rendering
 *
 * Layout (64 × 32 example, 4 departure rows fit neatly):
 *
 *   Row 0   ┌─ EASTBOUND ────────────────────────┐  ← header (white on dim bg)
 *           │                                     │
 *   Row 1   │  Route  XX min (HH:MM)              │  ← green / yellow / gray
 *           │                                     │
 *   Row 2   │  Route  XX min (HH:MM)              │
 *           │  ...                                │
 *   Row 3   │  Route  XX min (HH:MM)              │
 *   Row 4   │  Route  XX min (HH:MM)              │
 *           └─────────────────────────────────────┘
 *
 * Colour rules (same semantic meaning as the console version):
 *     ≤  5 min  →  GREEN
 *      6–12 min →  YELLOW
 *     unknown   →  GRAY  ("...")
 *     route name→  WHITE always
 *     header    →  WHITE text, DIM background strip
 * --------------------------------------------------------------------------- */
void render_display(DepartureList *list)
{
    if (!list) return;

    fb_clear();

    /* ── header ────────────────────────────────────────────────────────── */
    /* Dim background strip for the header row */
    for (int x = 0; x < MATRIX_WIDTH; x++)
        fb_set(x, 0, CLR_DIM);

    fb_draw_string(2, 0, "EASTBOUND", CLR_WHITE);

    /* If no departures, show a message and flush */
    if (list->count == 0) {
        fb_draw_string(2, LINE_HEIGHT, "No departures", CLR_GRAY);
        fb_flush();
        return;
    }

    /* ── timezone (same as print_output) ───────────────────────────────── */
    setenv("TZ", "America/Chicago", 1);
    tzset();
    time_t now = time(NULL);

    /* ── departure rows ────────────────────────────────────────────────── */
    /*
     * How many rows fit?  Each row is LINE_HEIGHT (10 px) pixels tall.
     * First data row starts at y = LINE_HEIGHT (below the header).
     * Max rows = (MATRIX_HEIGHT - LINE_HEIGHT) / LINE_HEIGHT
     */
    size_t max_rows = (MATRIX_HEIGHT - LINE_HEIGHT) / LINE_HEIGHT;
    size_t count    = list->count < max_rows ? list->count : max_rows;

    for (size_t i = 0; i < count; i++) {
        BusDeparture *d   = &list->items[i];
        int           y   = (int)(LINE_HEIGHT + i * LINE_HEIGHT);
        time_t        arrival = d->arrival_unix_time;

        /* ── route name (left-aligned, always white) ──────────────────── */
        int x_after_route = fb_draw_string(2, y, d->route_name, CLR_WHITE);

        /* ── arrival info ──────────────────────────────────────────────── */
        if (arrival == 0) {
            /* Unknown arrival — draw "..." in gray */
            fb_draw_string(x_after_route + 2, y, "...", CLR_GRAY);
            continue;
        }

        long mins = (arrival - now + 59) / 60; /* round up, same as print */

        if (mins < 0 || mins > 12 * 60) {
            /* Out of range — treat same as unknown */
            fb_draw_string(x_after_route + 2, y, "...", CLR_GRAY);
            continue;
        }

        /* Pick colour based on minutes remaining */
        RGB time_colour = (mins <= 5) ? CLR_GREEN : CLR_YELLOW;

        /* Build the two segments:  "XX min"  and  "(HH:MM)" */
        char buf_min[12];   /* e.g. " 3 min" */
        char buf_time[8];   /* e.g. "(14:07)" */
        struct tm tm;
        localtime_r(&arrival, &tm);

        snprintf(buf_min,  sizeof(buf_min),  "%ld min", mins);
        snprintf(buf_time, sizeof(buf_time), "(%02d:%02d)",
                 tm.tm_hour, tm.tm_min);

        /* Right-align the clock portion to column MATRIX_WIDTH - 2 */
        int x_time = calc_right_x(buf_time, MATRIX_WIDTH - 2);
        fb_draw_string(x_time, y, buf_time, CLR_GRAY);

        /* "XX min" sits between the route name and the clock */
        fb_draw_string(x_after_route + 2, y, buf_min, time_colour);
    }

    /* ── push to hardware ──────────────────────────────────────────────── */
    fb_flush();
}