// WORKING ON:
// fix abort in finish
// fix scroll and movement
// search and jump to next search position

// READY
// fix resize in (tmux?)
// insert mode: fix move corsor behind the last char and insert at the end

// BACKLOG:
// goto line number
// maybe Bug when row is empty (shows only #, which is but the sign for overscrolling a line aut of visible area)
// : jump to the end or the beginning of line
// move cursor by word
// move cursor by paragraph
// insert at position
// append to Line
// append new line
// move cursor in insert mode (emacs commands, arrow keys)
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


#define MAX_ESC_SEQ_LEN 255

#define INITIAL_STRING_BUFFER_SIZE 1
#define GOTO_FINISH(_ret_val) { ret_val = (_ret_val); goto finish; }

jmp_buf try;

Display d;
Editor e;
Viewport editor_viewport;  //= {2,2         ,d->lines-2,d->cols-2, (Scroll){0,0}};
Viewport search_viewport;  //= {0,d->lines-1,1       ,d->cols, (Scroll){0,0}};
Viewport message_viewport; //= {0,d->lines-1,1       ,d->cols, (Scroll){0,0}};

void rerender_all()
{
    display_resize(&d);

    switch(e.mode)
    {
        case browse:
        {
            fprintf(stderr, "BROWSE MODE\n");
            // adjust viewport after resize
            editor_viewport.lines = d.lines-2;
            editor_viewport.cols = d.cols-2;
            editor_render(&e, &editor_viewport, &d);
            break;
        }
        case search:
        {
            fprintf(stderr, "SEARCH MODE\n");
            // reserve bottom row for search word edditing
            // and clearly adjust to resize
            editor_viewport.lines = d.lines-2-1;
            editor_viewport.cols = d.cols-2;
            editor_render(&e, &editor_viewport, &d);

            search_viewport.y0 = d.lines-1;
            search_viewport.cols = d.cols;
            searchfield_render(e.search_field, &search_viewport, &d);
            break;
        }
        case insert:
        {
            fprintf(stderr, "INSERT MODE\n");
            // reserve bottom row for search word edditing
            // and clearly adjust to resize
            editor_viewport.lines = d.lines-2-1;
            editor_viewport.cols = d.cols-2;
            editor_render(&e, &editor_viewport, &d);

            message_viewport.y0 = d.lines-1;
            message_viewport.cols = d.cols;
            editor_message_render(&e, &message_viewport, &d);
            break;
        }
        default:
            fprintf(stderr,"ERROR: unknown editor mode\n");
            longjmp(try,EXIT_FAILURE);
    }
    display_render_to_terminal(&d);
}

// helper function to get escape sequence by pressing keys
int printt_escape_seq(void)
{
    char seq[MAX_ESC_SEQ_LEN];
    memset(seq, 0, sizeof(seq));
    int ret = read(STDIN_FILENO, seq, sizeof(seq));
    if (ret < 0) {
        fprintf(stderr, "ERROR: something went wrong during reading user input: %s\n", strerror(errno));
        return 1;
    }
    assert(ret < 32);
    printf("\"");
    for (int i = 0; i < ret; ++i) {
        printf("\\x%02x", seq[i]);
    }
    printf("\"\n");
    return 0;
}


MAYBE_TYPE(size_t)
MAYBE_FAIL(size_t)


// File Handling
bool file_exists(const char *path)
{
  struct stat s;   
  return stat(path, &s) == 0;
}

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
void handle_resize_signal()
{
    // TODO 
    // maybe better handle resizing here instead of in the game loop?
    fprintf(stderr, "handling resize\n");
    rerender_all();
}

typedef struct 
{
    EMode mode;
    char* seq;
    void (*handler)(Editor*, const char* s);
} KeyHandler;

void handle_quit(Editor *e, const char* s)
{
    e->mode = quit;
}
void handle_go_right(Editor *e, const char* s)
{
    if(e->mode==browse)
    {
        if(!(e->crsr.col >= e->lines[e->crsr.line].filled_size))
        {
            e->crsr.col += 1;
        }
        else
        {
            e->crsr.col = e->lines[e->crsr.line].filled_size;
        }
    }
    if(e->mode==insert)
    {

        if(!(e->crsr.col >= e->lines[e->crsr.line].filled_size))
        {
            e->crsr.col += 1;
        }   
    }

}
void handle_go_left(Editor *e, const char* s)
{
    if(e->crsr.col > 0) e->crsr.col -= 1;
}
void handle_go_down(Editor *e, const char* s)
{
    if(e->crsr.line < e->total_size-1)
    {
        e->crsr.line += 1;
    }
    //e->crsr.col = MIN(e->crsr.col, MAX(0,e->lines[e->crsr.line].filled_size));
}
void handle_go_up(Editor *e, const char* s)
{
    if(e->crsr.line > 0)
    {
        e->crsr.line -= 1;
    }
    //e->crsr.col = MIN(e->crsr.col, MAX(0,e->lines[e->crsr.line].filled_size));
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
    editor_insert(e, s);
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
void handle_delete_insert_mode(Editor *e, const char *s)
{
    assert(0 && "unimplemented");
}
void handle_leave_insert_mode(Editor *e, const char *s)
{                                    
    e->mode = browse;                                    
    editor_set_message(e, NULL); // means delete message
}                                    

int main(int argc, char* argv[])
{
    int ret_val = 0;
    char *line = NULL;
    KeyHandler handler_map[] = {
        {browse, "q", handle_quit},
        {browse, "l", handle_go_right},
        {browse, "h", handle_go_left},
        {browse, "j", handle_go_down},
        {browse, "k", handle_go_up},
        {browse, "\x1b\x5b\x44", handle_go_left},
        {browse, "\x1b\x5b\x43", handle_go_right},
        {browse, "\x1b\x5b\x41", handle_go_up},
        {browse, "\x1b\x5b\x42", handle_go_down},
        {browse, "s", handle_enter_search_mode},
        {browse, "i", handle_enter_insert_mode},
        {insert, "\x1b\x5b\x44", handle_go_left},
        {insert, "\x1b\x5b\x43", handle_go_right},
        {insert, "\x1b\x5b\x41", handle_go_up},
        {insert, "\x1b\x5b\x42", handle_go_down},
        {insert, "\x7f", handle_delete_insert_mode},
        {insert, "\x04", handle_delete_insert_mode},
        {insert, ESCAPE, handle_leave_insert_mode},
        {search, ESCAPE, handle_leave_search_mode},
        {search, "\x7f", handle_delete_search_mode},
        {search, "\x04", handle_delete_search_mode},
        {search, "\x1b\x5b\x44", handle_left_search_mode},
        {search, "\x1b\x5b\x43", handle_right_search_mode},
    };

    assert(argc == 2 && "ERROR: you have to provide a file name");
    if(!file_exists(argv[1])) 
    {
        fprintf(stderr, "ERROR: file does not exists %s.\n", strerror(errno));
        GOTO_FINISH(1);
    }

    MAYBE(size_t) size = file_size(argv[1]);
    if(IS_NOTHING2(size))
    {
        fprintf(stderr, "ERROR cannot determin file size %s.\n", strerror(errno));
        GOTO_FINISH(1);
    }
    if(MAYBE_VALUE_ACCESS(size) == 0)
    {
        fprintf(stderr, "ERROR File is empty.\n");
        GOTO_FINISH(1);
    }

    if(EXIT_FAILURE == editor_init(&e)) 
    {
        fprintf(stderr, "ERROR: failed to allocate editor.\n");
        GOTO_FINISH(1);     
    } 

    FILE* f = fopen(argv[1], "rb");
    if(!f)
    {
        fprintf(stderr, "ERROR: failed to open filed %s. Reaseon: %s.\n", argv[1], strerror(errno));
        GOTO_FINISH(1);
    }

    size_t line_len = 80;
    line = malloc(sizeof(char)*line_len);
    while(!feof(f))
    {
        ssize_t ret = getline(&line, &line_len, f);
        if(ret == -1)
        {
            fprintf(stderr, "ERROR: failed to read file: %s.\n", strerror(errno));
            GOTO_FINISH(1);
        }
        if(!editor_append_line(&e, line))
        {
            fprintf(stderr, "ERROR: failed to load file.\n");
            GOTO_FINISH(1);
        }
    }

    // TODO get Terminal size to initialise display size
    if(EXIT_FAILURE == display_init(&d, 20,20))
    {
        fprintf(stderr, "ERROR: failed to allocate display.\n");
        GOTO_FINISH(1);     
    } 

    bool terminal_ready = false;

    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) 
    {
        fprintf(stderr, "ERROR: Program has to be started in real terminal!\n");
        GOTO_FINISH(1);
    }
    struct termios term;
    if (tcgetattr(STDIN_FILENO, &term) < 0) 
    {
        fprintf(stderr, "ERROR: could not determin the state of the term: %s\n", strerror(errno));
        GOTO_FINISH(1);
    }

    term.c_lflag &= ~ECHO;
    term.c_lflag &= ~ICANON;
    if (tcsetattr(0, 0, &term)) 
    {
        fprintf(stderr, "ERROR: could not update the state of the terminal: %s\n", strerror(errno));
        GOTO_FINISH(1);
    }

    terminal_ready = true;

    struct sigaction act, old = {0};
    act.sa_handler = handle_resize_signal;
    if (sigaction(SIGWINCH, &act, &old) < 0) {
        fprintf(stderr, "ERROR: could not set resize handler: %s\n", strerror(errno));
        GOTO_FINISH(1);
    }

    //bool quit = false;
    //rerender_all();
    //if(!display_resize(&d)) GOTO_FINISH(1)

    editor_viewport  = (Viewport){2,2         ,d.lines-2,d.cols-2, (Scroll){0,0}};
    search_viewport  = (Viewport){0,d.lines-1,1       ,d.cols, (Scroll){0,0}};
    message_viewport = (Viewport){0,d.lines-1,1       ,d.cols, (Scroll){0,0}};

    // using lonjump to be able to handle exceptions from resize handler appropriatelly
    // kind of try-catch with no syntactic sugar
    int catch_exception = setjmp(try);
    if(catch_exception==EXIT_FAILURE)
    {
        fprintf(stderr, "ERROR: caught an error\n");
        GOTO_FINISH(1);
    }
    while (e.mode != quit) 
    {
        //uncomment to see ascii sequence while typing
        //if((ret_val=printt_escape_seq()) != 0) GOTO_FINISH(ret_val)
        //else continue;

        fprintf(stderr, "TRACE: begin of game loop\n");

        rerender_all();
        //switch(e->mode)
        //{
        //    case browse:
        //    {
        //        fprintf(stderr, "BROWSE MODE\n");
        //        // adjust viewport after resize
        //        editor_viewport.lines = d->lines-2;
        //        editor_viewport.cols = d->cols-2;
        //        editor_render(e, &editor_viewport, d);
        //        break;
        //    }
        //    case search:
        //    {
        //        fprintf(stderr, "SEARCH MODE\n");
        //        // reserve bottom row for search word edditing
        //        // and clearly adjust to resize
        //        editor_viewport.lines = d->lines-1;
        //        editor_viewport.cols = d->cols;
        //        editor_render(e, &editor_viewport, d);

        //        search_viewport.y0 = d->lines-1;
        //        search_viewport.cols = d->cols;
        //        searchfield_render(e->search_field, &search_viewport, d);
        //        break;
        //    }
        //    case insert:
        //    {
        //        fprintf(stderr, "INSERT MODE\n");
        //        // reserve bottom row for search word edditing
        //        // and clearly adjust to resize
        //        editor_viewport.lines = d->lines-1;
        //        editor_viewport.cols = d->cols;
        //        editor_render(e, &editor_viewport, d);

        //        message_viewport.y0 = d->lines-1;
        //        message_viewport.cols = d->cols;
        //        editor_message_render(e, &message_viewport, d);
        //        break;
        //    }
        //    default:
        //        fprintf(stderr,"ERROR: unkown editor mode\n");
        //        GOTO_FINISH(1);
        //}
        //display_render_to_terminal(d);

        char seq[MAX_ESC_SEQ_LEN] = {0};
        errno = 0;
        int seq_len = read(STDIN_FILENO, seq, sizeof(seq));
        if (errno == EINTR) 
        {
            // there was a signal from resize not an entry
            // resize is handled in the resize handler completly
            // so ignore this here and loop
            //if(EXIT_FAILURE == display_resize(&d)){
            //    fprintf(stderr, "ERROR: failed to resize\n");
            //    GOTO_FINISH(1);
            //} 
            continue;
        }
        if (errno > 0) 
        {
            fprintf(stderr, "ERROR: error while reading input: %s\n", strerror(errno));
            GOTO_FINISH(1);
        }

        assert((size_t) seq_len < sizeof(seq));

        void (*key_handler)(Editor*, const char*) = NULL;
        for(size_t i=0; i < LEN(handler_map); i++)
        {
            if(   handler_map[i].mode == e.mode
               && strcmp(seq, handler_map[i].seq) == 0)
            {
                key_handler = handler_map[i].handler;
            }
        }
        if(key_handler) {
            key_handler(&e, seq);
        }
        else
        {
            // no special handler found try gegenric
            bool ok = true;
            switch (e.mode)
            {
            case search:
                for(size_t i=0; i<strlen(seq); i++) ok &= isprint(seq[i]);
                if(ok) handle_generic_searchmode(&e, seq);
                break;
            case insert:
                for(size_t i=0; i<strlen(seq); i++) ok &= isprint(seq[i]);
                if(ok) handle_generic_insertmode(&e, seq);
                break;
            default:
                break;
            }

        }
        fprintf(stderr, "TRACE: end of game loop\n");
    }
finish:
    fprintf(stderr, "TRACE: result from setjmp %s\n", catch_exception==EXIT_FAILURE? "EXIT_FAILURE": "EXIT_SUCCESS");
    fprintf(stderr, "TRACE: editor mode==%s\n", e.mode==quit? "quit": "not quit");
    fprintf(stderr, "TRACE: finishing program ret_val==%d\n", ret_val);
    if (terminal_ready) 
    {
        // reset treminal 
        printf("\033[2J");
        term.c_lflag |= ECHO;
        term.c_lflag |= ICANON;
        tcsetattr(STDIN_FILENO, 0, &term);
    }
    if(line) free(line);
    editor_free(&e);
    display_free(&d);
    if(f) fclose(f);
    return ret_val;
}