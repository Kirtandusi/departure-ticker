#ifndef NETWORKING_H
#define NETWORKING_H

#include <stddef.h>

char *get_data(const char *url, size_t *out_size);
char *build_url(const char *api_key, char *stop_ids[], size_t n_stops);

#endif
