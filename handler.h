#ifndef __HANDLER_H__
#define __HANDLER_H__
//#include <stdlib.h>
//#include <stdio.h>
//#include <string.h>
//#include <stdbool.h>
//#include <termios.h>
//#include <unistd.h>
//#include <errno.h>
//#include <assert.h>
//#include <sys/stat.h>
//#include <sys/ioctl.h>
//#include <signal.h>
//#include <ctype.h>
//#include <stdbool.h>
//#include <setjmp.h>
#include "maybe.h"
#include "defs.h"
//#include "display.h"
#include "editor.h"

typedef struct 
{
    EMode mode;
    char* seq;
    void (*handler)(Editor*, const char* s);
} KeyHandler;

// TODO separate File Handling 
MAYBE_TYPE(size_t)
bool file_exists(const char *path);
MAYBE(size_t) file_size(const char *path);

// TODO separate rendering
void rerender_all(      Display *d,
                  const Editor   *e,
                        Viewport *editor_viewport,
                        Viewport *search_viewport,
                        Viewport *message_viewport);

//void handle_resize_signal(int signal)
//void handle_resize_signal();

// browse mode
void handle_enter_insert_mode(Editor *e, const char *s);
void handle_backspace_browse_mode(Editor *e, const char *s);
void handle_delete_browse_mode(Editor *e, const char *s);
void handle_delete_line_browse_mode(Editor *e, const char *s);
void handle_browse_cr(Editor *e, const char *s);
void handle_browse_to_end_of_file(Editor *e, const char *s);
void handle_browse_to_end_of_line(Editor *e, const char *s);
void handle_browse_to_begin_of_file(Editor *e, const char *s);
void handle_browse_to_begin_of_line(Editor *e, const char *s);

// insert mode
void handle_insert_cr(Editor *e, const char *s);
void handle_left_insert_mode (Editor *e, const char *s);
void handle_right_insert_mode(Editor *e, const char *s);
void handle_backspace_insert_mode(Editor *e, const char *s);
void handle_delete_insert_mode(Editor *e, const char *s);
void handle_leave_insert_mode(Editor *e, const char *s);
void handle_generic_insertmode(Editor *e, const char*s);

// search mode
void handle_enter_search_mode(Editor *e, const char*s);
void handle_leave_search_mode(Editor *e, const char*s);
void handle_generic_searchmode(Editor *e, const char*s);
void handle_delete_search_mode(Editor *e, const char*s);
void handle_left_search_mode(Editor *e, const char*s);
void handle_right_search_mode(Editor *e, const char*s);
void handle_search_next(Editor *e, const char *s);


// generic
void handle_quit(Editor *e, const char* s);
void handle_go_right(Editor *e, const char* s);
void handle_go_left(Editor *e, const char* s);
void handle_go_down(Editor *e, const char* s);
void handle_go_up(Editor *e, const char* s);
void handle_save(Editor *e, const char *s);
void handle_goto_line(Editor *e, int l);

#endif