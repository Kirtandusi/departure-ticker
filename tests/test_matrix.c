/* The real upstream C header with a fake panel: no GPIO access in tests. */
#include "render.h"
#include <led-matrix-c.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct RGBLedMatrix { int unused; };
struct LedCanvas { int unused; };
struct LedFont { int unused; };
static struct RGBLedMatrix fake_matrix;
static struct LedCanvas fake_canvas;
static struct LedFont fake_font;
static char drawn[16][64];
static unsigned draw_count, swaps, deleted;

struct RGBLedMatrix *led_matrix_create_from_options(struct RGBLedMatrixOptions *options,
                                                    int *argc, char ***argv) {
    (void)argc; (void)argv;
    assert(options->cols == 64 && options->rows == 32 && options->chain_length == 1);
    return &fake_matrix;
}
struct LedCanvas *led_matrix_create_offscreen_canvas(struct RGBLedMatrix *matrix) {
    assert(matrix == &fake_matrix);
    return &fake_canvas;
}
void led_canvas_get_size(const struct LedCanvas *canvas, int *width, int *height) {
    assert(canvas == &fake_canvas);
    *width = 64; *height = 32;
}
struct LedFont *load_font(const char *path) { assert(path); return &fake_font; }
int height_font(struct LedFont *font) { assert(font == &fake_font); return 7; }
int baseline_font(struct LedFont *font) { assert(font == &fake_font); return 6; }
int character_width_font(struct LedFont *font, uint32_t codepoint) {
    assert(font == &fake_font); (void)codepoint; return 5;
}
void led_canvas_clear(struct LedCanvas *canvas) {
    assert(canvas == &fake_canvas);
    draw_count = 0;
}
int draw_text(struct LedCanvas *canvas, struct LedFont *font, int x, int y,
              uint8_t r, uint8_t g, uint8_t b, const char *text, int kerning) {
    assert(canvas == &fake_canvas && font == &fake_font);
    assert(x >= 0 && x + (int)strlen(text) * 5 <= 64 && y >= 0 && y < 32);
    assert(draw_count < 16);
    (void)r; (void)g; (void)b; (void)kerning;
    snprintf(drawn[draw_count++], sizeof(drawn[0]), "%s", text);
    return (int)strlen(text) * 5;
}
struct LedCanvas *led_matrix_swap_on_vsync(struct RGBLedMatrix *matrix, struct LedCanvas *canvas) {
    assert(matrix == &fake_matrix && canvas == &fake_canvas);
    ++swaps;
    return canvas;
}
void led_matrix_delete(struct RGBLedMatrix *matrix) { assert(matrix == &fake_matrix); ++deleted; }
void delete_font(struct LedFont *font) { assert(font == &fake_font); }

int main(void) {
    char *args[] = {"ticker", NULL};
    char **argv = args;
    int argc = 1;
    assert(render_init(&argc, &argv, 1) == 0);
    BusDeparture items[] = {{"0454", "A", 1600000060}, {"0133", "B", 1600000300},
                            {"0857", "C", 0}, {"10062", "D", 1600000600}};
    DepartureList list = {items, 4};
    render_display(&list, 1600000000, 0, 0);
    assert(!strcmp(drawn[0], "BUS 1/2"));
    assert(!strcmp(drawn[1], "A") && !strcmp(drawn[2], "1m"));
    assert(!strcmp(drawn[5], "0857") && !strcmp(drawn[6], "--"));
    render_display(&list, 1600000000, 2, 0);
    assert(!strcmp(drawn[1], "0454"));
    render_display(&list, 1600000000, 4, 0);
    assert(!strcmp(drawn[0], "BUS 2/2"));
    assert(!strcmp(drawn[1], "D") && !strcmp(drawn[2], "10m"));
    render_display(&list, 1600000000, 0, 1);
    assert(!strcmp(drawn[0], "DEMO 1/2"));
    render_error();
    assert(draw_count == 2 && !strcmp(drawn[0], "OFFLINE"));
    assert(swaps == 5);
    render_shutdown();
    render_shutdown();
    assert(deleted == 1);
    puts("Matrix regressions passed (64x32 layout, pages, labels, offline, cleanup).");
    return 0;
}
