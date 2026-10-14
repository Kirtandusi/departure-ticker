#ifndef CONFIG_H
#define CONFIG_H
#include "parser.h"

/* Missing files are fine. Invalid lines fail without logging their contents. */
int load_dotenv(const char *path);
int parse_stops(const char *text, char storage[MAX_STOPS][STOP_ID_SIZE],
                char *stops[MAX_STOPS], size_t *count);
int poll_interval(const char *text, unsigned *seconds);
#endif
