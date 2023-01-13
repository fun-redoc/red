#ifndef __EDITOR_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <signal.h>
#include "maybe.h"
#include "defs.h"
#include "cursor.h"
#include "display.h"

typedef enum
{
    quit,
    search,
    browse,
    insert
} EMode;
typedef struct {
    char* content;
    size_t total_size;
    size_t filled_size;
} Line;

typedef struct {
    Line* lines;
    size_t total_size;
    Cursor crsr;
    Cursor viewportOffset;
    EMode mode;
} Editor;

Editor* editor_init();
void editor_free(Editor *e);
bool editor_append_line(Editor *e, const char *s);
void editor_render_to_display(const Editor *e, Display *d);;
#endif