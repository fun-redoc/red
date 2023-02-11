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
#include <ctype.h>
#include <stdbool.h>
#include <setjmp.h>
#include "maybe.h"
#include "defs.h"
#include "display.h"
#include "editor.h"
#include "handler.h"


extern jmp_buf try; // TODO alternatively pass try pointer in all methods as param

void rerender_all(      Display *d,
                  const Editor   *e,
                        Viewport *editor_viewport,
                        Viewport *search_viewport,
                        Viewport *message_viewport) 
{
    display_resize(d);

    switch(e->mode)
    {
        case browse:
        {
            // adjust viewport after resize
            editor_viewport->lines = d->lines-2;
            editor_viewport->cols = d->cols-2;
            editor_render(e, editor_viewport, d);
            break;
        }
        case search:
        {
            // reserve bottom row for search word edditing
            // and clearly adjust to resize
            editor_viewport->lines = d->lines-2-1;
            editor_viewport->cols = d->cols-2;
            editor_render(e, editor_viewport, d);

            search_viewport->y0 = d->lines-1;
            search_viewport->cols = d->cols;
            searchfield_render(e->search_field, search_viewport, d);
            break;
        }
        case insert:
        {
            // reserve bottom row for search word edditing
            // and clearly adjust to resize
            editor_viewport->lines = d->lines-2-1;
            editor_viewport->cols = d->cols-2;
            editor_render(e, editor_viewport, d);

            message_viewport->y0 = d->lines-1;
            message_viewport->cols = d->cols;
            editor_message_render(e, message_viewport, d);
            break;
        }
        default:
            fprintf(stderr,"ERROR: unknown editor mode\n");
            longjmp(try,EXIT_FAILURE);
    }
    display_render_to_terminal(d);
}

// File Handling
bool file_exists(const char *path)
{
  struct stat s;   
  return stat(path, &s) == 0;
}

MAYBE_FAIL(size_t)
MAYBE(size_t) file_size(const char *path)
{
    struct stat s;   
    if(stat(path, &s) == 0)
    {
        INIT_SOME(size_t, res, s.st_size);
        return res;
    }
    else
    {
        INIT_NOTHING(size_t, res);
        return res;
    }
}

//void handle_resize_signal(int signal)
//void handle_resize_signal()
//{
//    // TODO 
//    // maybe better handle resizing here instead of in the game loop?
//    fprintf(stderr, "handling resize\n");
//    rerender_all();
//}


void handle_quit(Editor *e, const char* s)
{
    e->mode = quit;
}
void handle_go_right(Editor *e, const char* s)
{
    e->crsr.col += 1;
    e->crsr.col = MIN(e->crsr.col, MAX(0,e->lines[e->crsr.line].filled_size));
}
void handle_go_left(Editor *e, const char* s)
{
    if(e->crsr.col > 0) e->crsr.col -= 1;
}
void handle_go_down(Editor *e, const char* s)
{
    if(e->crsr.line < e->count-1)
    {
        e->crsr.line += 1;
    }
    e->crsr.col = MIN(e->crsr.col, MAX(0,e->lines[e->crsr.line].filled_size));
}
void handle_go_up(Editor *e, const char* s)
{
    if(e->crsr.line > 0)
    {
        e->crsr.line -= 1;
    }
    e->crsr.col = MIN(e->crsr.col, MAX(0,e->lines[e->crsr.line].filled_size));
}
void handle_enter_search_mode(Editor *e, const char*s)
{
    assert(e);
    if(!e->search_field)
    {
        e->search_field = searchfield_init();
    }
    e->mode = search;
}
void handle_leave_search_mode(Editor *e, const char*s)
{
    e->mode = browse;
}

void handle_generic_searchmode(Editor *e, const char*s)
{
    searchfield_edit(e->search_field, s);
}
void handle_delete_search_mode(Editor *e, const char*s)
{
    searchfield_delete(e->search_field);
}
void handle_left_search_mode(Editor *e, const char*s)
{
    searchfield_move_left(e->search_field);
}
void handle_right_search_mode(Editor *e, const char*s)
{
    searchfield_move_right(e->search_field);
}

void handle_generic_insertmode(Editor *e, const char*s)
{
    editor_insert(e, s, strlen(s));
}
void handle_enter_insert_mode(Editor *e, const char *s)
{                                    
    e->mode = insert;                                    
    editor_set_message(e, MESSAGE_INSERT_MODE);
}                                    
void handle_left_insert_mode (Editor *e, const char *s)
{                                    
    assert(0 && "unimplemented");
}                                    
void handle_right_insert_mode(Editor *e, const char *s)
{                                    
    assert(0 && "unimplemented");
}
void handle_backspace_browse_mode(Editor *e, const char *s)
{
    editor_backspace_at_crsr(e);
}
void handle_backspace_insert_mode(Editor *e, const char *s)
{
    editor_backspace_at_crsr(e);
}
void handle_delete_browse_mode(Editor *e, const char *s)
{
    editor_delete_at_crsr(e);
}
void handle_delete_insert_mode(Editor *e, const char *s)
{
    editor_delete_at_crsr(e);
}
void handle_leave_insert_mode(Editor *e, const char *s)
{                                    
    e->mode = browse;                                    
    editor_set_message(e, NULL); // means delete message
}                                    

void handle_browse_cr(Editor *e, const char *s)
{
    if(e->crsr.line+1 < e->count)
    {
        e->crsr.line += 1;
        e->crsr.col = 0;
    }
}

void handle_insert_cr(Editor *e, const char *s)
{
    // TODO starting from here there is a seg fault
    Line *l = &(e->lines[e->crsr.line]);
    char *r = &(l->content[e->crsr.col]);
    if(l->filled_size < e->crsr.col) return;
    size_t len = l->filled_size - e->crsr.col;
    size_t next_line = e->crsr.line + 1;
    // TODO 
    // 1. count and capacity 
    // 2. move code to editor.c using a smart function name
    if(next_line <= e->count)
    {
        // insert line
        size_t needed_capacity = e->count + 1;
        if(needed_capacity > e->capacity)
        {
            e->lines = realloc(e->lines, sizeof(Line)*(needed_capacity));
            if(!e->lines) return;
            e->capacity = needed_capacity;
        }
        size_t new_size = e->count + 1;
        // TODO think on using memmove
        /*
        1aaaaa (crsr.col = 2 crsr.line = 1)
        2bbbbb <-
        3ccccc
        4ddddd (total_size = 4; new_size = 5; next_line = 2)
           ::becomes
        1aaaaa
        2bb    
        bbb    <- next_line
        3ccccc <- end of copy
        4ddddd <- new_size-1
        */
        for(size_t i=new_size-1; i>=next_line+1; i--)
        {
            e->lines[i] = e->lines[i-1];
        }
        e->lines[next_line].content = malloc(sizeof(char)*len);
        e->lines[next_line].filled_size = len;
        e->lines[next_line].total_size = len;
        memmove(e->lines[next_line].content, 
                &(e->lines[e->crsr.line].content[e->crsr.col]),
                len);
        memset(&(e->lines[e->crsr.line].content[e->crsr.col]), '\0', len);
        e->lines[e->crsr.line].filled_size -= len;
        /*
        L1: 123456789
                ^
            L1:total_size  = 11
            L1:filled_size = 9
            crsr.col = 4
            len = filled_size - crsr.col = 9 - 4 = 5
        =>becomes
        123456789
        1234.....
        56789
        ^
            L1:total_size = 11
            l1:filled_size = old filled_size - len = 9 - 5=4
        */
        e->count = new_size;
        e->crsr.line += 1;
        e->crsr.col = 0;
    }
    else
    {
        // append line
        assert(0 && "not yet implementd");
    }


}

void handle_browse_to_end_of_file(Editor *e, const char *s)
{
    e->crsr.line = e->count-1;
    e->crsr.col = e->lines[e->crsr.line].filled_size;
    fprintf(stderr, "continuing handle_browse_to_end_of_file %zu %zu\n",e->crsr.line, e->crsr.col);
    fprintf(stderr, "%s\n",e->lines[e->crsr.line].content);
}

void handle_browse_to_end_of_line(Editor *e, const char *s)
{
    fprintf(stderr, "in handle_browse_to_end_of_line %zu %zu %zu\n",e->count, e->crsr.line, e->crsr.col);
    if(e->crsr.line < e->count)
    {
        if(e->lines[e->crsr.line].filled_size > 0)
        {
            e->crsr.col = e->lines[e->crsr.line].filled_size-1;
        }
    }
    fprintf(stderr, "in handle_browse_to_end_of_line %zu %zu\n",e->crsr.line, e->crsr.col);
}

void handle_browse_to_begin_of_line(Editor *e, const char *s)
{
    e->crsr.col = 0;
}
void handle_browse_to_begin_of_file(Editor *e, const char *s)
{
    e->crsr.col = 0;
    e->crsr.line = 0;
}

void handle_save(Editor *e, const char *s)
{                                    
    assert(e && e->filepath);
    FILE *f = fopen(e->filepath, "wb");
    //FILE *f = fopen("save.test.txt", "wb");
    if(!f)
    {
        fprintf(stderr, "ERROR: failed to open file %s for writing errno=%d (%s).\n", e->filepath, errno, strerror(errno));
        longjmp(try, EXIT_FAILURE);
    }
    for(size_t i=0; i < e->count; i++)
    {
        if(EOF == fputs(e->lines[i].content, f))
        {
            fprintf(stderr, "ERROR: failed to save file %s for writing errno=%d (%s).\n", e->filepath, errno, strerror(errno));
            longjmp(try, EXIT_FAILURE);
        }
        if(EOF == fputc('\n', f))
        {
            fprintf(stderr, "ERROR: failed to save file %s for writing errno=%d (%s).\n", e->filepath, errno, strerror(errno));
            longjmp(try, EXIT_FAILURE);
        }
    }
    fclose(f);
}                                    