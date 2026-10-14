#ifndef RENDER_H
#define RENDER_H
#include "parser.h"

/* Returns minutes, 0 for due, or -1 for no usable departure. */
long departure_minutes(const BusDeparture *departure, time_t now);
int render_init(int *argc, char ***argv, int enabled);
void render_display(const DepartureList *list, time_t now, unsigned page_tick, int demo);
void render_error(void);
void render_shutdown(void);
void print_output(const DepartureList *list, time_t now);
#endif
