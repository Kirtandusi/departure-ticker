#ifndef RENDER_H
#define RENDER_H

#include "parser.h"   /* BusDeparture, DepartureList */

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