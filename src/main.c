#include <stdio.h>
#include <stdlib.h>
#include "networking.h"
#include "parser.h"
#include "render.h"

int main() {
    printf("Fetching bus data...\n");
    char *json = get_data();

    if (json) {
        parse_json(json);
        free(json);
    }

    render_display();

    return 0;
}
