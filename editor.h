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
    char *filepath;
    EMode mode;
    Line* lines;
    size_t count;
    size_t capacity;
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
void editor_insert(Editor *e, const char *s, size_t len);
void editor_delete_at_crsr(Editor *e);
void editor_delete_line_at_crsr(Editor *e);
void editor_backspace_at_crsr(Editor *e);

void editor_message_set(Editor *e, const char *s);
void editor_message_render(const Editor *e, const Viewport *v, Display *d);
bool editor_message_check(const Editor *e);

bool editor_search_next(Editor *e);
Cursor editor_next_word(Editor *e);
Cursor editor_prev_word(Editor *e);

bool editor_equals(const Editor *e1, const Editor *e2);

#endif