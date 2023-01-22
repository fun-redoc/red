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
#define LINE_GROTH_FACTOR 2

int editor_init(Editor* e)
{
    e->lines = NULL;
    e->total_size = 0;
    e->crsr = (Cursor){0,0};
    e->viewportOffset = (Cursor){0,0};
    e->mode = browse;
    e->search_field = NULL;
    e->message = NULL;
    e->message_capacity = 0;
    e->message_count = 0;
    return EXIT_SUCCESS;
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
    }
}

bool editor_append_line(Editor *e, const char *s)
{
    assert(e && s);

    size_t new_size;
    if(e->lines == NULL)
    {
        new_size = 1;
        e->lines = malloc(sizeof(Line));
    }
    else
    {
        new_size = e->total_size + 1;
        e->lines = realloc(e->lines, sizeof(Line)*(new_size));
    }
    if(e->lines == NULL) return false;
    e->total_size = new_size;

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
    fprintf(stderr, "TRACE: enter editor_render  !(%zu + %zu <= %zu)\n",v->x0,v->cols, disp->cols);
    fprintf(stderr, "TRACE:                      !(%zu + %zu <= %zu)\n",v->y0,v->lines, disp->lines);
    assert(e!=NULL && disp!=NULL && v!=NULL);
    // viewport has to fit into Display
    if(!(v->x0 + v->cols <= disp->cols)) return;
    if(!(v->y0 + v->lines <= disp->lines)) return;

    fprintf(stderr, "TRACE: in editor_render after checks \n");

//    size_t cols_available  = viewport->cols  - viewport->x0;
//    size_t lines_available = viewport->lines - viewport->y0;
    
    // prepare cursors
    // TODO maybe vieportOffset (scolling offset) ist better located in Display instead of Editor
    //scroll to the right if necessary
    if(e->crsr.col >= v->scrollOffset.cols + v->cols)
    {
        if(v->scrollOffset.cols + v->cols <= e->lines[e->crsr.line].filled_size)
        {
            v->scrollOffset.cols += 1;
            fprintf(stderr, "TRACE: editor_render 1 -- scrl.col=%zu\n", v->scrollOffset.cols);
        }
    }
    //scroll to the left if necessary
    if(e->crsr.col < v->scrollOffset.cols)
    {
        if(e->crsr.col < v->cols)
        {
            v->scrollOffset.cols -= 1;
        }
        fprintf(stderr, "TRACE: editor_render 2 -- scrl.col=%zu\n", v->scrollOffset.cols);
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

    fprintf(stderr, "d->lines=%zu, v-lines=%zu\n", disp->lines, v->lines);
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
            //size_t dlen = MIN(v->cols, MAX(0, e->lines[el].filled_size - ec));
            fprintf(stderr, "TRACE: editor_render 3 -- scrl.col=%zu dlen=%zu\n", ec, dlen);
            if(dlen > 0) 
                memcpy(&(disp->viewbuffer[(v->y0 + l)*disp->cols+ (v->x0)]), &(e->lines[el].content[ec]), dlen);
            if(dlen == 0) 
                disp->viewbuffer[(v->y0 + l)*disp->cols + (v->x0)] = SCROLLED_OUT_OF_SIGHT;

        }
        else
        {
            fprintf(stderr, "TRACE: editor_render 4\n");
            disp->viewbuffer[(v->y0 + l)*disp->cols + (v->x0)] = EPMTY_LINE;
        }

        // for debugging purpose uncomment
        //memset(&(disp->viewbuffer[(v->y0 + l)*disp->cols+(v->x0)]), '#', v->cols);
    } 

    // set cursor in scroll
    int crsr_col = e->crsr.col - v->scrollOffset.cols; // can get negative?
    fprintf(stderr, "TRACE crsr_col=%d\n", crsr_col);
    disp->crsr.col  = v->x0 + MAX(0,crsr_col);
    disp->crsr.line = e->crsr.line - v->scrollOffset.lines +v->y0;
    fprintf(stderr, "TRACE: leaving editor_render\n");
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
        memset(&(d->viewbuffer[(v->y0)*d->cols+(v->x0)]), ' ', v->cols);
        memcpy(&(d->viewbuffer[(v->y0)*d->cols+(v->x0)]), e->message, max_contet_len);
    }
}

void editor_insert(Editor *e, const char *s)
{
    assert(e);
    assert(s && strlen(s) > 0);
    if(e->total_size > e->crsr.line)
    {
        // edit exising line
        #define line (e->lines[e->crsr.line])
        size_t needed_capcacity = strlen(s) + line.filled_size;
        if(needed_capcacity < line.total_size)
        {
            fprintf(stderr, "-> no resize needed - line capacity %zu, needed %zu", line.total_size, needed_capcacity);
            // entry fits current allocated line space
            assert(e->crsr.col+1 < line.total_size);
            size_t n = line.filled_size - e->crsr.col;
            memcpy(&(line.content[e->crsr.col+strlen(s)]), &(line.content[e->crsr.col]),n);
            memcpy(&(line.content[e->crsr.col]), s, strlen(s));
            e->crsr.col += strlen(s);
            line.filled_size += strlen(s);
        }
        else
        {
            fprintf(stderr, "-> cur line |%s|\n", line.content);
            fprintf(stderr, "-> line capacity %zu, count %zu\n", line.total_size, line.filled_size);
            fprintf(stderr, "-> resize needed - line capacity %zu, needed %zu\n", line.total_size, needed_capcacity);
            size_t new_total_size = line.total_size * LINE_GROTH_FACTOR;
            while(new_total_size < needed_capcacity) new_total_size *= LINE_GROTH_FACTOR; 
            fprintf(stderr, "-> new capcacity %zu\n", new_total_size);
            line.content = realloc(line.content, sizeof(char)*new_total_size);
            if(!line.content)
            {
                fprintf(stderr, "ERROR: failed to alloc memory, cannot insert (%d) %s\n", errno, strerror(errno));
                return;
            }
            line.total_size = new_total_size;
            fprintf(stderr, "-> cur line extended |%s|\n", line.content);
            fprintf(stderr, "-> line capacity %zu, count %zu\n", line.total_size, line.filled_size);
            size_t n = line.filled_size - e->crsr.col;
            fprintf(stderr, "-> bytes to shift %zu\n", n);
            fprintf(stderr, "-> crsr at %zu, to insert %zu\n", e->crsr.col, strlen(s));
            memcpy(&(line.content[e->crsr.col+strlen(s)]), &(line.content[e->crsr.col]),n);
            memcpy(&(line.content[e->crsr.col]), s, strlen(s));
            e->crsr.col += strlen(s);
            line.filled_size += strlen(s);
        }
    }
    else
    {
        // TODO append line
        assert(0 && "not yetz implemented");
    }
}

#endif