#ifndef __EDITOR_H__
#define __EDITOR_H__
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
#include "defs.h"
#include "maybe.h"
#include "display.h"
#include "search_field.h"

typedef enum
{
    quit,
    search,
    browse,
    insert,
} EMode;
typedef struct {
    char* content;
    size_t total_size;
    size_t filled_size;
} Line;

typedef struct {
    EMode mode;
    Line* lines;
    size_t total_size;
    Cursor crsr;
    Cursor viewportOffset;
    SearchField *search_field;
    char *message;
    size_t message_capacity;
    size_t message_count;
} Editor;

/**
 * @brief 
 * 
 * @param e 
 * @return EXIT_FAILURE or EXIT_SUCCESS
 */
int editor_init(Editor* e);

void editor_free(Editor *e);
bool editor_append_line(Editor *e, const char *s);
void editor_render(const Editor *e, Viewport *viewport, Display *d);
void editor_insert(Editor *e, const char *s);

void editor_set_message(Editor *e, const char *s);
void editor_message_render(const Editor *e, const Viewport *v, Display *d);

#endif