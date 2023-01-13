#ifndef __DISPLAY_H__
#define __DISPLAY_H__
#include <stdlib.h>
#include "maybe.h"
#include "cursor.h"
#include "editor.h"

typedef struct {
    size_t lines;
    size_t cols;
} Scroll;

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
void display_render_editor(Display *d, Editor *e);

#endif