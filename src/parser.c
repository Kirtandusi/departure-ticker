#include "parser.h"
#include <stdio.h>

void parse_json(const char *json) {
    if (!json) {
        printf("No JSON to parse\n");
        return;
    }

    // TODO: Implement real parsing later
    printf("Received JSON (%zu bytes)\n", strlen(json));
}
