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

int display_init(Display *d, const size_t lines, const size_t cols)
{
    assert(d &&lines != 0 && cols != 0);
    size_t buf_size = cols*lines;
    d->viewbuffer = malloc(sizeof(char)*buf_size);
    if(d->viewbuffer==NULL)
    {
        free(d);
        return EXIT_FAILURE;
    }
    d->lines = lines;
    d->cols = cols;
    d->crsr = (Cursor){0,0};
    d->scrollOffset = (Scroll){0,0};
    //fill with blakns
    memset(d->viewbuffer,'*', buf_size);
    return EXIT_SUCCESS;
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

int display_resize(Display *d)
{
    struct winsize w;
    int err = ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    if(err != 0)
    {
        fprintf(stderr,"ERROR: Failed to receive signal, ioctl terminated with Error %d (%s).\n", err, strerror(err));
        return EXIT_FAILURE;
    }
    d->lines = w.ws_row;
    d->cols = w.ws_col;
    d->viewbuffer = realloc(d->viewbuffer, d->lines*d->cols*sizeof(*d->viewbuffer));
    if(!d->viewbuffer)
    {
        fprintf(stderr,"ERROR: Failed to reallocate viewbuffer %d (%s).\n", errno, strerror(errno));
        return EXIT_FAILURE;
    }
    memset(d->viewbuffer,'*', d->lines*d->cols);
    return EXIT_SUCCESS;
}


void display_render_to_terminal(const Display *d)
{
    fprintf(stdout, ANSI_HOME);
    fwrite(d->viewbuffer, sizeof(*d->viewbuffer), d->lines*d->cols, stdout);
    fprintf(stdout, ANSI_CURSOR_XY, d->crsr.line + 1, d->crsr.col + 1);
    fflush(stdout);
}

