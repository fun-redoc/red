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
#include "cursor.h"
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


void display_render_editor(Display *d, Editor *e)
{
    assert(e!=NULL && d!=NULL);
    
    // prepare cursors
    // TODO maybe vieportOffset (scolling offset) ist better located in Display instead of Editor
    //scroll to the right if necessary
    if(e->crsr.col - e->viewportOffset.col >= d->cols)
    {
        if(e->viewportOffset.col + d->cols < e->lines[e->crsr.line].filled_size)
        {
            e->viewportOffset.col += 1;
        }
    }
    //scroll to the left if necessary
    if(e->crsr.col < e->viewportOffset.col)
    {
        e->viewportOffset.col -= 1;
    }
    // scroll down
    if(e->crsr.line > d->lines-1)
    {
        if(e->total_size - e->viewportOffset.line > d->lines-1)
        {
            e->viewportOffset.line +=1;
        }
    }
    // scroll up
    if(e->crsr.line < e->viewportOffset.line)
    {
        e->viewportOffset.line =e->crsr.line;
    }

    // clear viewport
    memset(d->viewbuffer, BLANK, d->lines*d->cols);

    // render viewport
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

    // set cursor in viewport
    d->crsr.col  = e->crsr.col - e->viewportOffset.col;
    d->crsr.line = e->crsr.line - e->viewportOffset.line;
}