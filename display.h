#ifndef __DISPLAY_H__
#define __DISPLAY_H__
#include <stdlib.h>
#include <stdbool.h>
#include "defs.h"
#include "maybe.h"


typedef struct {
    char *viewbuffer;
    size_t lines;
    size_t cols;
    Cursor crsr;
    Scroll scrollOffset;
} Display;

Display* display_init(const size_t lines, const size_t cols);
void display_free(Display *d);
bool display_resize(Display *d);
void display_render_to_terminal(const Display *d);

#endif