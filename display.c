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
    d->scrollOffset = (Scroll){0,0};
    //fill with blakns
    memset(d->viewbuffer,'*', buf_size);
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
    memset(d->viewbuffer,'*', d->lines*d->cols);
    return true;
}


void display_render_to_terminal(const Display *d)
{
    fprintf(stdout, ANSI_HOME);
    fwrite(d->viewbuffer, sizeof(*d->viewbuffer), d->lines*d->cols, stdout);
    fprintf(stdout, ANSI_CURSOR_XY, d->crsr.line + 1, d->crsr.col + 1);
    fflush(stdout);
}

