// WORKING ON:
// TODO search and jump to next search position

// READY

// BACKLOG:
// TODO maybe Bug when row is empty (shows only #, which is but the sign for overscrolling a line aut of visible area)
// TODO : jump to the end or the beginning of line
// TODO move cursor by word
// TODO move cursor by paragraph
// TODO insert mode
// TODO insert at position
// TODO append to Line
// TODO append new line
// TODO move cursor in insert mode (emacs commands, arrow keys)
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

#define NEW_LINE  '\n'
#define BLANK     '\x20'
#define ESCAPE    "\x1b"
#define BACKSPACE "\x7f"
#define DELETE    "\x1b\x5b\x33\x7e"

#define MAX_ESC_SEQ_LEN 255
#define LINE_INITIAL_SIZE 1 // increas for "procductive" usage

#define DO_TRACE true
#define TRACE_TO stdout
#define TRACE_DO(do_this) { if(DO_TRACE) do_this; }

#define LEN(a) (sizeof((a))/sizeof((a[0])))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

#define INITIAL_STRING_BUFFER_SIZE 1
#define GOTO_FINISH(_ret_val) { ret_val = (_ret_val); goto finish; }

typedef enum
{
    quit,
    search,
    browse,
    insert
} EMode;

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

typedef struct {
    size_t line, col;
} Cursor;

typedef struct {
    char *viewbuffer;
    size_t lines;
    size_t cols;
    Cursor crsr;
} Display;

typedef struct {
    char *viewbuffer;
    size_t lines;
    size_t cols;
    Cursor crsr;
} Display;
Display* display_init(const size_t lines, const size_t cols)
{
    assert(lines != 0 && cols != 0);
    size_t buf_size = cols*lines;
    Display *d = malloc(sizeof(Display));
    if(d==NULL) return NULL;
    d->viewbuffer = malloc(sizeof(char)*buf_size);
    if(d->viewbuffer==NULL)
    {
        free(d);
        return NULL;
    }
    d->lines = lines;
    d->cols = cols;
    d->crsr = (Cursor){0,0};
    //fill with blakns
    memset(d->viewbuffer,BLANK, buf_size);
    return d;
}

void display_free(Display *d)
{
    if(d)
    {
        if(d->viewbuffer) free(d->viewbuffer);
        d->lines = 0;
        d->cols  = 0;
        d->crsr  = (Cursor){0,0};
    }
}


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

Editor* editor_init()
{
    Editor *e;
    e = malloc(sizeof(Editor));
    e->lines = NULL;
    e->total_size = 0;
    e->crsr = (Cursor){0,0};
    e->viewportOffset = (Cursor){0,0};
    e->mode = browse;
    return e;
}

void editor_free(Editor *e)
{
    if(e != NULL)
    {
        if(e->lines != NULL)
        {
            for(int i=0; i<e->total_size; i++)
            {
                free(e->lines[i].content);
                e->lines[i].content = NULL;
                e->lines[i].filled_size = 0;
                e->lines[i].total_size = 0;
            }
            free(e->lines);
            e->lines = NULL;
        }
        free(e);
        e = NULL;
    }
}

bool editor_append_line(Editor *e, const char *s)
{
    assert(e && s);

    if(e->lines == NULL)
    {
        e->total_size = 1;
        e->lines = malloc(sizeof(Line));
    }
    else
    {
        e->total_size += 1;
        e->lines = realloc(e->lines, sizeof(Line)*(e->total_size));
    }
    if(e->lines == NULL) return false;

    size_t l = e->total_size - 1;
    size_t slen = strlen(s);
    if(s[slen-1] == '\n') slen -= 1; // skip new line
    e->lines[l].total_size = MAX(LINE_INITIAL_SIZE, slen);

    e->lines[l].content = malloc(sizeof(char)*(e->lines[l].total_size));
    if(!e->lines[l].content) return false;

    memcpy(e->lines[l].content, s, slen);
    e->lines[l].filled_size = slen;

    return true;
}

void editor_render_to_display(const Editor *e, Display *d)
{
    assert(e!=NULL && d!=NULL);
    memset(d->viewbuffer, BLANK, d->lines*d->cols);
    for(size_t l=0; l<d->lines; l++)
    {
        size_t dl = e->viewportOffset.line + l;
        size_t dc = e->viewportOffset.col;
        if(dl < e->total_size)
        {
            size_t dlen = MIN(d->cols, e->lines[dl].filled_size <= dc ? 0 : e->lines[dl].filled_size - dc);
            if(dlen > 0) memcpy(&(d->viewbuffer[l*d->cols]), &(e->lines[dl].content[dc]), dlen);
            if(dlen == 0) d->viewbuffer[l*d->cols] = '#';
        }
        else
        {
            d->viewbuffer[l*d->cols] = '~';
        }
    } 
    d->crsr.col  = e->crsr.col - e->viewportOffset.col;
    d->crsr.line = e->crsr.line - e->viewportOffset.line;
}

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

bool display_resize(Display *d)
{
    struct winsize w;
    int err = ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    if(err != 0)
    {
        fprintf(stderr,"ERROR: Failed to receive signal, ioctl terminated with Error %d (%s).\n", err, strerror(err));
        return false;
    }
    d->lines = w.ws_row;
    d->cols = w.ws_col;
    d->viewbuffer = realloc(d->viewbuffer, d->lines*d->cols*sizeof(*d->viewbuffer));
    if(!d->viewbuffer)
    {
        fprintf(stderr,"ERROR: Failed to reallocate viewbuffer %d (%s).\n", errno, strerror(errno));
        return false;
    }
    return true;
}


void display_render_to_terminal(const Display *d)
{
    fprintf(stdout, "\033[H");
    fwrite(d->viewbuffer, sizeof(*d->viewbuffer), d->lines*d->cols, stdout);
    fprintf(stdout, "\033[%zu;%zuH", d->crsr.line + 1, d->crsr.col + 1);
    fflush(stdout);
}

//void handle_resize_signal(int signal)
void handle_resize_signal()
{
    // TODO 
    // maybe better handle resizing here instead of in the game loop?
}

typedef struct 
{
    EMode mode;
    char* seq;
    void (*handler)(Editor*, Display*);
} KeyHandler;

void handle_quit(Editor *e, Display *d)
{
    e->mode = quit;
}
void handle_go_right(Editor *e, Display *d)
{
    e->crsr.col += 1;
    if(e->crsr.col - e->viewportOffset.col >= d->cols)
    {
        if(e->viewportOffset.col + d->cols < e->lines[e->crsr.line].filled_size)
        {
            e->viewportOffset.col += 1;
        }
    }
    if(e->crsr.col  >= e->lines[e->crsr.line].filled_size)
    {
        e->crsr.col = e->lines[e->crsr.line].filled_size - 1;
    }
}
void handle_go_left(Editor *e, Display *d)
{
    if(e->crsr.col > 0) e->crsr.col -= 1;
    if(e->crsr.col < e->viewportOffset.col)
    {
        e->viewportOffset.col -= 1;
    }
}
void handle_go_down(Editor *e, Display *d)
{
    if(e->crsr.line < e->total_size-1)
    {
        e->crsr.line += 1;
    }
    if(e->crsr.line > d->lines-1)
    {
        if(e->total_size - e->viewportOffset.line > d->lines-1)
        {
            e->viewportOffset.line +=1;
        }
    }
    e->crsr.col = MIN(e->crsr.col, MAX(1,e->lines[e->crsr.line].filled_size)-1);
    fprintf(stderr, "e->crsr.col=%zu; line_filledsize=%zu\n", e->crsr.col,e->lines[e->crsr.line].filled_size);
}
void handle_go_up(Editor *e, Display *d)
{
    if(e->crsr.line > 0)
    {
        e->crsr.line -= 1;
    }
    if(e->crsr.line < e->viewportOffset.line)
    {
        e->viewportOffset.line =e->crsr.line;
    }
    e->crsr.col = MIN(e->crsr.col, MAX(1,e->lines[e->crsr.line].filled_size)-1);
}
void handle_go_up(Editor *e, Display *d)
{
    e->mode = search;
}


KeyHandler handler_map[] = {
    {browse, "q", handle_quit},
    {browse, "l", handle_go_right},
    {browse, "h", handle_go_left},
    {browse, "j", handle_go_down},
    {browse, "k", handle_go_up},
    {browse, "s", handle_start_search_mode},
//    {search, ENTER, handle_tart_search_mode},
};

int main(int argc, char* argv[])
{
    int ret_val = 0;
    char *line = NULL;

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

    Editor* e = editor_init();
    if(!e)
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
        if(!editor_append_line(e, line))
        {
            fprintf(stderr, "ERROR: failed to load file.\n");
            GOTO_FINISH(1);
        }
    }

    // TODO get Terminal size to initialise display size
    Display *d = display_init(20,20);

    bool insert = false;
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

    bool quit = false;
    if(!display_resize(d)) GOTO_FINISH(1)
    while (e->mode != quit) 
    {
//        if((ret_val=printt_escape_seq()) != 0) GOTO_FINISH(ret_val)
        editor_render_to_display(e, d);
        display_render_to_terminal(d);

        char seq[MAX_ESC_SEQ_LEN] = {0};
        errno = 0;
        int seq_len = read(STDIN_FILENO, seq, sizeof(seq));
        if (errno == EINTR) 
        {
            if(!display_resize(d)) GOTO_FINISH(1);
            continue;
        }
        if (errno > 0) 
        {
            fprintf(stderr, "ERROR: error while reading input: %s\n", strerror(errno));
            GOTO_FINISH(1);
        }

        assert((size_t) seq_len < sizeof(seq));

        void (*key_handler)(Editor*, Display*) = NULL;
        for(size_t i=0; i < LEN(handler_map); i++)
        {
            if(   handler_map[i].mode == e->mode
               && strcmp(seq, handler_map[i].seq) == 0)
            {
                key_handler = handler_map[i].handler;
            }
        }
        if(key_handler) key_handler(e,d);
    }

finish:
    if (terminal_ready) 
    {
        // reset treminal 
        printf("\033[2J");
        term.c_lflag |= ECHO;
        term.c_lflag |= ICANON;
        tcsetattr(STDIN_FILENO, 0, &term);
    }
    if(line) free(line);
    editor_free(e);
    display_free(d);
    if(f) fclose(f);
    return ret_val;
}
