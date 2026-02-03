#ifndef RENDER_H
#define RENDER_H

#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stddef.h>
#include <time.h>

/* ---------------------------------------------------------------------------
 * Data types  (keep in sync with whatever your fetch layer produces)
 * --------------------------------------------------------------------------- */
typedef struct {
    char   route_name[16];
    time_t arrival_unix_time;   /* 0  = unknown / no ETA */
} BusDeparture;

typedef struct {
    BusDeparture *items;
    size_t        count;
} DepartureList;

/* ---------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------------- */

/*
 * matrix_init()  – MUST be called once at startup, before render_display().
 *                  Initialises the hzeller rpi-rgb-led-matrix library,
 *                  creates the canvas, starts the refresh thread.
 *                  Returns 0 on success, -1 on failure.
 */
int  matrix_init(void);

/*
 * matrix_cleanup() – call at exit to stop the refresh thread and free memory.
 */
void matrix_cleanup(void);

/* Console renderer (unchanged) */
void print_output(DepartureList *list);

/* RGB matrix renderer – draws one frame and pushes it to the panel */
void render_display(DepartureList *list);

#endif /* RENDER_H */