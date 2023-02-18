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
    e->filepath = NULL;
    e->lines = NULL;
    e->count = 0;
    e->capacity = 0;
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
        if(e->filepath)
        {
            free(e->filepath);
            e->filepath = NULL;
        }
        if(e->search_field)
        {
            searchfield_free(e->search_field);
            free(e->search_field);
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
            for(int i=0; i<e->capacity; i++)
            {
                assert(!e->lines[i].content || e->lines[i].total_size > 0);
                assert(e->lines[i].total_size > 0 || !e->lines[i].content);
                if(e->lines[i].content) free(e->lines[i].content);
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

    size_t new_count;
    size_t new_capacity;
    if(e->lines == NULL)
    {
        new_count    = 1;
        new_capacity = 1;
        e->lines     = malloc(sizeof(Line));
        if(e->lines) 
        {
            e->lines[new_capacity-1] = (Line){NULL,0,0};
        }
    }
    else
    {
        new_count = e->count + 1;
        if(new_count > e->capacity)
        {
            new_capacity = e->capacity +1;
            e->lines = realloc(e->lines, sizeof(Line)*(new_capacity));
            if(e->lines)
            {
                e->lines[new_capacity-1] = (Line){NULL,0,0};
            }
        }
    }
    if(e->lines == NULL) return false;
    e->capacity = new_capacity;
    e->count    = new_count;

    size_t l = e->count - 1;
    size_t slen = strlen(s);
    if(s[slen-1] == '\n') slen -= 1; // skip new line
    size_t new_total_size = MAX(LINE_INITIAL_SIZE, slen);
    if(e->lines[l].content == NULL)
    {
        assert(e->lines[l].total_size == 0);
        e->lines[l].content = malloc(sizeof(char)*(new_total_size));
        memset(e->lines[l].content, '\0', new_total_size);
    }
    else
    {
        assert(e->lines[l].total_size > 0);
        e->lines[l].content = realloc(e->lines[l].content, sizeof(char)*(new_total_size));
    }
    if(!e->lines[l].content) return false;
    e->lines[l].total_size = new_total_size;

    memcpy(e->lines[l].content, s, slen);
    e->lines[l].filled_size = slen;

    return true;
}

//bool old_editor_append_line(Editor *e, const char *s)
//{
//    assert(e && s);
//
//    size_t new_size;
//    if(e->lines == NULL)
//    {
//        new_size = 1;
//        e->lines = malloc(sizeof(Line));
//    }
//    else
//    {
//        new_size = e->total_size + 1;
//        e->lines = realloc(e->lines, sizeof(Line)*(new_size));
//    }
//    if(e->lines == NULL) return false;
//    e->total_size = new_size;
//
//    size_t l = e->total_size - 1;
//    size_t slen = strlen(s);
//    if(s[slen-1] == '\n') slen -= 1; // skip new line
//    e->lines[l].total_size = MAX(LINE_INITIAL_SIZE, slen);
//
//    e->lines[l].content = malloc(sizeof(char)*(e->lines[l].total_size));
//    if(!e->lines[l].content) return false;
//
//    memcpy(e->lines[l].content, s, slen);
//    e->lines[l].filled_size = slen;
//
//    return true;
//}

void editor_render(const Editor *e, Viewport *v, Display *disp)
{
    fprintf(stderr, "entering editor_render\n");
    assert(e!=NULL && disp!=NULL && v!=NULL);
    // viewport has to fit into Display
    if(!(v->x0 + v->cols <= disp->cols)) return;
    if(!(v->y0 + v->lines <= disp->lines)) return;
    
    fprintf(stderr, "continuing editor_render crsr.col=%zu\n", e->crsr.col);

    // prepare cursors
    // TODO maybe vieportOffset (scolling offset) ist better located in Display instead of Editor
    //scroll to the right if necessary
    if(e->crsr.col >= v->scrollOffset.cols + v->cols)
    {
        if(v->scrollOffset.cols + v->cols <= e->lines[e->crsr.line].filled_size)
        {
            v->scrollOffset.cols = e->crsr.col  - v->cols;
            //v->scrollOffset.cols += 1;
        }
    }
    //scroll to the left if necessary
    if(e->crsr.col >= 0 && 
       e->crsr.col < v->scrollOffset.cols)
    {
        v->scrollOffset.cols = e->crsr.col;
    }
    // scroll down
    if(e->crsr.line > v->lines-1)
    {
        if(e->count - v->scrollOffset.lines >= v->lines-1)
        {
            fprintf(stderr, "continuing editor_render - scroll down\n");
            v->scrollOffset.lines = e->crsr.line - v->lines +1;
            //v->scrollOffset.lines +=1;
        }
    }
    // scroll up
    if(e->crsr.line < v->scrollOffset.lines)
    {
        fprintf(stderr, "continuing editor_render - scroll up\n");
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

        if(el < e->count)
        {
            size_t dlen = MIN(v->cols, e->lines[el].filled_size <= ec ? 0 : e->lines[el].filled_size - ec);
            if(dlen > 0) 
                memcpy(&(disp->viewbuffer[(v->y0 + l)*disp->cols+ (v->x0)]), &(e->lines[el].content[ec]), dlen);
            if(dlen == 0) 
                disp->viewbuffer[(v->y0 + l)*disp->cols + (v->x0)] = SCROLLED_OUT_OF_SIGHT;
        }
        else
        {
            disp->viewbuffer[(v->y0 + l)*disp->cols + (v->x0)] = EPMTY_LINE;
        }

        // for debugging purpose uncomment
        //memset(&(disp->viewbuffer[(v->y0 + l)*disp->cols+(v->x0)]), '#', v->cols);
    } 

    // set cursor in scroll
    fprintf(stderr, "before crsr.line=%zu, crsr.col=%zu\n", e->crsr.line, e->crsr.col);
    int crsr_col = e->crsr.col - v->scrollOffset.cols; // can get negative?
    disp->crsr.col  = v->x0 + MAX(0,crsr_col);
    disp->crsr.line = e->crsr.line - v->scrollOffset.lines +v->y0;
    fprintf(stderr, "after crsr.line=%zu, crsr.col=%zu\n", e->crsr.line, e->crsr.col);
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
        size_t max_contet_len = MIN(e->message_count, v->cols);
        memset(&(d->viewbuffer[(v->y0)*d->cols+(v->x0)]), ' ', v->cols);
        memcpy(&(d->viewbuffer[(v->y0)*d->cols+(v->x0)]), e->message, max_contet_len);
    }
}

void editor_insert(Editor *e, const char *s, size_t len)
{
    assert(e);
    assert(s != NULL && len > 0 && strnlen(s, len) > 0);
    fprintf(stderr, "hier 1\n");
    if(e->count > e->crsr.line)
    {
        fprintf(stderr, "hier 2\n");
        // edit exising line
        #define cur_line (e->lines[e->crsr.line])
        //size_t needed_capcacity = strlen(s) + cur_line.filled_size;
        size_t needed_capcacity = len + cur_line.filled_size;
        if(needed_capcacity < cur_line.total_size)
        {
            fprintf(stderr, "hier 3\n");
            
            // entry fits current allocated cur_line.space
            assert(e->crsr.col+1 < cur_line.total_size);
            size_t n = cur_line.filled_size - e->crsr.col;
            //jmemcpy(&(cur_line.content[e->crsr.col+strlen(s)]), &(cur_line.content[e->crsr.col]),n);
            memcpy(&(cur_line.content[e->crsr.col+len]), &(cur_line.content[e->crsr.col]),n);
            //memcpy(&(cur_line.content[e->crsr.col]), s, strlen(s));
            memcpy(&(cur_line.content[e->crsr.col]), s, len);
            //e->crsr.col += strlen(s);
            e->crsr.col += len;
            //cur_line.filled_size += strlen(s);
            cur_line.filled_size += len;
        }
        else
        {
            fprintf(stderr, "hier 4\n");
            size_t new_total_size = MAX(1, cur_line.total_size * LINE_GROTH_FACTOR);
            while(new_total_size < needed_capcacity) new_total_size *= LINE_GROTH_FACTOR; 
            
            fprintf(stderr, "hier 5\n");
            cur_line.content = realloc(cur_line.content, sizeof(char)*new_total_size);
            if(!cur_line.content)
            {
                fprintf(stderr, "ERROR: failed to alloc memory, cannot insert (%d) %s\n", errno, strerror(errno));
                return;
            }
            cur_line.total_size = new_total_size;
            
            size_t n = cur_line.filled_size - e->crsr.col;
            
            //memcpy(&(cur_line.content[e->crsr.col+strlen(s)]), &(cur_line.content[e->crsr.col]),n);
            memcpy(&(cur_line.content[e->crsr.col+len]), &(cur_line.content[e->crsr.col]),n);
            //memcpy(&(cur_line.content[e->crsr.col]), s, strlen(s));
            memcpy(&(cur_line.content[e->crsr.col]), s, len);
            //e->crsr.col += strlen(s);
            e->crsr.col += len;
            //cur_line.filled_size += strlen(s);
            cur_line.filled_size += len;
        }
    }
    else
    {
        for(size_t i=e->count; i<=e->crsr.line-1; i++)
        {
            editor_append_line(e, "");
        }
        editor_append_line(e, s);
        e->crsr.col = 0;
        // TODO append line
        //assert(0 && "not yetz implemented");
    }
}

void editor_delete_at_xy(Editor *e, size_t x, size_t y)
{
    assert(e && e->lines);
    if(y>=0 && y< e->count)
    {
        if(e->lines[y].filled_size > x && x >= 0)
        {
            memmove(&(e->lines[y].content[x]),
                    &(e->lines[y].content[x+1]),
                    e->lines[y].filled_size - x - 1
            ); 
            e->lines[y].filled_size -= 1;
            e->lines[y].content[e->lines[y].filled_size] = '\0';
            assert(e->lines[y].filled_size >= 0);
        }
    }
}

void editor_backspace_at_crsr(Editor *e)
{
    if(e->crsr.col == 0)
    {
       if(e->crsr.line > 0)
       {
            size_t old_line_no = e->crsr.line;
            size_t old_col_no = e->crsr.col;
            e->crsr.line -= 1;
            e->crsr.col = e->lines[e->crsr.line].filled_size;
            size_t new_col_no = e->lines[e->crsr.line].filled_size;
            editor_insert(e, e->lines[old_line_no].content, e->lines[old_line_no].filled_size);
            e->crsr.col = new_col_no;
            free(e->lines[old_line_no].content);
            e->lines[old_line_no].content = NULL;
            e->lines[old_line_no].filled_size = 0;
            e->lines[old_line_no].total_size = 0;
            for(size_t l=old_line_no; l<e->count-1;l++)
            {
                e->lines[l] = e->lines[l+1];
            }
            e->lines[e->count-1].content = NULL;
            e->lines[e->count-1].filled_size = 0;
            e->lines[e->count-1].total_size = 0;
            e->count -= 1;
       }
    }
    else if(e->crsr.col > 0)
    {
        editor_delete_at_xy(e, e->crsr.col-1, e->crsr.line);
        if(e->crsr.col > 0) // necessary because maybe side effect in delete_at fn.
        {
            e->crsr.col -= 1;
        }
    }
}
void editor_delete_line_at_crsr(Editor *e)
{
    if(!e->lines) return;
    if(e->count <= 0) return;

    if(e->lines[e->crsr.line].total_size > 0 && e->lines[e->crsr.line].content)
    {
        free(e->lines[e->crsr.line].content);
    }
    for(size_t i=e->crsr.line; i<e->count-1; i++)
    {
        e->lines[i] = e->lines[i+1];
    }
    e->lines[e->count-1].content = NULL;
    if(e->count > 0)     e->count -= 1;
    if(e->crsr.line > 0) e->crsr.line -= 1;
    e->crsr.col = 0;
}
void editor_delete_at_crsr(Editor *e)
{
    if(!e->lines) return;
    if(e->count <= 0) return;
    if(e->lines[e->crsr.line].filled_size > 0)
    {
        editor_delete_at_xy(e, e->crsr.col, e->crsr.line);
        if(e->crsr.col >= e->lines[e->crsr.line].filled_size-1)
        {
            e->crsr.col = e->lines[e->crsr.line].filled_size-1;
        }
    }
    if(e->lines[e->crsr.line].filled_size == 0)
    {
        // empty line can be deleted
        if(e->lines[e->crsr.line].content) free(e->lines[e->crsr.line].content);
        // TODO better use memmove instead of loop
        for(size_t i=e->crsr.line; i<e->count-1;i++)
        {
            e->lines[i] = e->lines[i+1];
        }
        e->count -= 1;
        e->lines[e->count].content = NULL;
        e->lines[e->count].filled_size = 0;
        e->lines[e->count].total_size = 0;
    }
}

bool line_equal(const Line *l1, const Line *l2)
{
    //bool res = (l1 != NULL) => (l2 != NULL);
    bool res = !(l1 != NULL) || (l2 != NULL);
    if(res && l1 && l2)
    {
        //res &= l1->total_size == l2->total_size;
        res &= l1->filled_size == l2->filled_size;
        res &= (0 == strncmp(l1->content, l2->content, l1->filled_size));
    }
    return res;
}

bool editor_equals(const Editor *e1,const Editor *e2)
{
    //bool res = (e1 != NULL) => (e2 != NULL);
    bool res = !(e1 != NULL) || (e2 != NULL);
    if(res && e1 && e2)
    {
        res &= (0 == strcmp(e1->filepath, e2->filepath));
        res &= e1->crsr.col == e2->crsr.col && e1->crsr.line == e2->crsr.line;
        res &= e1->viewportOffset.col == e2->viewportOffset.col && e1->viewportOffset.line == e2->viewportOffset.line;
        res &= e1->count == e2->count;
        //res &= e1->capacity == e2->capacity;
        res &= e1->mode == e2->mode;
        res &= e1->message_capacity == e2->message_capacity;
        res &= e1->message_count == e2->message_count;
        res &= (0 == strncmp(e1->message, e2->message, e1->message_count));
        res &= searchfield_equal(e1->search_field, e2->search_field);

        if(res)
        {
            for(size_t i=0; res && i < e1->count; i++)
            {
                res &= line_equal(&(e1->lines[i]), &(e2->lines[i]));
            }
        }
    }

    return res;
}

#endif