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
#include "editor.h"

#define LINE_INITIAL_SIZE 1 // increas for "procductive" usage

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
#endif