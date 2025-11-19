#include "../include/render.h"
#include "../include/parser.h"
#include <stdio.h>

void render_display(DepartureList *list) {
     if (!list) {
        printf("Nothing to display.\n");
        return;
    }
    printf("Rendering display...\n");
    
    // TODO: draw bus times on the LED matrix
    //depending on bus, color changes. 
}
