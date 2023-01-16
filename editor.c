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
    e->search_field = NULL;
    e->message = NULL;
    e->message_capacity = 0;
    e->message_count = 0;
    return e;
}

void editor_free(Editor *e)
{
    if(e != NULL)
    {
        if(e->search_field)
        {
            searchfield_free(e->search_field);
            e->search_field = NULL;
        }
        if(e->message)
        {
            free(e->message);
            e->message = NULL;
            e->message_capacity = 0;
            e->message_count = 0;
        }
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

void editor_render(const Editor *e, Viewport *v, Display *disp)
{
    assert(e!=NULL && disp!=NULL && v!=NULL);
    // viewport has to fit into Display
    assert(v->cols <= disp->cols);
    assert(v->lines <= disp->lines);

//    size_t cols_available  = viewport->cols  - viewport->x0;
//    size_t lines_available = viewport->lines - viewport->y0;
    
    // prepare cursors
    // TODO maybe vieportOffset (scolling offset) ist better located in Display instead of Editor
    //scroll to the right if necessary
    if(e->crsr.col >= v->scrollOffset.cols + v->cols)
    {
        if(v->scrollOffset.cols + v->cols < e->lines[e->crsr.line].filled_size)
        {
            v->scrollOffset.cols += 1;
        }
    }
    //scroll to the left if necessary
    if(e->crsr.col < v->scrollOffset.cols)
    {
        v->scrollOffset.cols -= 1;
    }
    // scroll down
    if(e->crsr.line > v->lines-1)
    {
        if(e->total_size - v->scrollOffset.lines > v->lines-1)
        {
            v->scrollOffset.lines +=1;
        }
    }
    // scroll up
    if(e->crsr.line < v->scrollOffset.lines)
    {
        v->scrollOffset.lines =e->crsr.line;
    }

    // render scroll
    for(size_t l=0; l<v->lines; l++)
    {
        // position in editor
        size_t el = v->scrollOffset.lines + l;
        size_t ec = v->scrollOffset.cols;
        
        // clear the line first
        memset(&(disp->viewbuffer[(v->y0 + l)*disp->cols+(v->x0)]), BLANK, v->cols);

        if(el < e->total_size)
        {
            size_t dlen = MIN(v->cols, e->lines[el].filled_size <= ec ? 0 : e->lines[el].filled_size - ec);
            if(dlen > 0) memcpy(&(disp->viewbuffer[(v->y0 + l)*disp->cols+ (v->x0)]), &(e->lines[el].content[ec]), dlen);
            if(dlen == 0) disp->viewbuffer[(v->y0 + l)*disp->cols + (v->x0)] = SCROLLED_OUT_OF_SIGHT;

        }
        else
        {
            disp->viewbuffer[(v->y0 + l)*disp->cols + (v->x0)] = EPMTY_LINE;
        }
    } 

    // set cursor in scroll
    disp->crsr.col  = e->crsr.col  - v->scrollOffset.cols + v->x0;
    disp->crsr.line = e->crsr.line - v->scrollOffset.lines +v->y0;
}

void editor_set_message(Editor *e, const char *s)
{
    if(!s || strlen(s) == 0)
    {
        // clear message
        if(e->message)
        {
            memset(e->message, BLANK, strlen(e->message));
            e->message_count = 0;
        }
    }
    else
    {
        if(!e->message)
        {
            // lasily allocate
            e->message = malloc(sizeof(char)*strlen(s));
            if(!e->message)
            {
                fprintf(stderr, "ERROR: failed to allocate memory for message (%d)%s\n.", errno, strerror(errno));
                fprintf(stderr, "       trying to continue. eaving and restarting recommended.\n");
                return;
            }
            memcpy(e->message, s, strlen(s));
            e->message_capacity = strlen(s);
            e->message_count = e->message_capacity;
        }
        else
        {
            // already allocated
            if(strlen(s) > e->message_capacity)
            {
                // need more space
                e->message = realloc(e->message, strlen(s));
                if(!e->message)
                {
                    fprintf(stderr, "ERROR: failed to (re)allocate memory for message (%d)%s\n.", errno, strerror(errno));
                    fprintf(stderr, "       trying to continue. saving and restarting recommended.\n");
                    return;
                }
                e->message_capacity = strlen(s);
                e->message_count = e->message_capacity;
                memcpy(e->message, s, strlen(s));
            }
            else
            {
                memset(e->message, BLANK, e->message_capacity);
                memcpy(e->message, s, strlen(s));
                e->message_count = strlen(s);
            }
        }
    }
}

void editor_message_render(const Editor *e, const Viewport *v, Display *d)
{
    assert(e && v && d);
    assert(v->lines == 1);

    if(!(e->message && strlen(e->message)>0)) return;
    
    assert(strlen(e->message) == e->message_capacity);
    assert(e->message_count <= e->message_capacity);

    // TODO check if viewport fits with other viewport elsewhere
    if(d->cols >= v->x0 + v->cols &&
       d->lines >= (v->y0 + v->lines) && 
       v->cols > e->message_count)
    {
        //fprintf(stderr, "TRACE: rendering possible\n");
        size_t max_contet_len = MIN(e->message_count, v->cols);
        memset(&(d->viewbuffer[(v->y0)*d->cols+(v->x0)]), ' ', max_contet_len);
        memcpy(&(d->viewbuffer[(v->y0)*d->cols+(v->x0)]), e->message, max_contet_len);
    }
}

void editor_insert(Editor *e, const char *s)
{
    assert(e);
    assert(s && strlen(s) > 0);
    if(e->total_size < e->crsr.line)
    {
        // edit exising line
        Line line = e->lines[e->crsr.line];
        size_t needed_capcacity = strlen(s) + line.filled_size;
        if(needed_capcacity < line.total_size)
        {
            // entry fits current allocated line space
//            memmove(line.content[e->crsr])
        }
        else
        {
            // TODO rsize line
        }
    }
    else
    {
        // TODO append line
    }
    assert(0 && "not yet implemented");
}

#endif