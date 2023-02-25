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
#include "maybe.h"
#include "defs.h"
#include "display.h"
#include "editor.h"

#define LINE_INITIAL_SIZE 1 // increas for "procductive" usage
#define LINE_GROTH_FACTOR 2

const char *substr_no_case(const char *hay, size_t hay_len, const char *needle, size_t needle_len)
{
    const char *res;
    if(!hay) return NULL;
    if(!needle) return NULL;

    size_t len = needle_len;
    if(hay_len < 1) return NULL;
    if(len < 1) return NULL;
    if(hay_len < len) return NULL;

    char *s1 = calloc((hay_len+1), sizeof(char));
    if(!s1){
        fprintf(stderr, "Failed to alloc memory in %s:%d\n", __FILE__, __LINE__);
        res = NULL;
        goto finish;
    } 
    char *s2 = calloc(len+1, sizeof(char));
    if(!s2){
        fprintf(stderr, "Failed to alloc memory in %s:%d\n", __FILE__, __LINE__);
        res = NULL;
        goto finish;
    } 

    strncpy(s1, hay, hay_len);
    s1[hay_len] ='\0';
    // to lower
    for(char *p=s1; *p; p++) *p=tolower(*p);
    
    strncpy(s2, needle, len);
    s2[len] ='\0';
    // to lower
    for(char *p=s2; *p; p++) *p=tolower(*p);

    const char *found = strstr(s1,s2);
    if(!found){
        res = NULL;
        goto finish;
    } 

    size_t found_pos = found-s1;
    res = &(hay[found_pos]);
finish:
    if(s1) free(s1);
    if(s2) free(s2);
    return res;
}

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

void editor_render(const Editor *e, Viewport *v, Display *disp)
{

    assert(e!=NULL && disp!=NULL && v!=NULL);
    // crop cursor to used area
    Cursor crsr = e->crsr;
    if(crsr.line >= e->count)
    {
        crsr.line = e->count;
        if(crsr.line > 0) crsr.line -= 1;
    }
    //if(e->count > 0 && e->lines != NULL && crsr.col >= e->lines[crsr.line].filled_size)
    if(e->count > 0 && e->lines != NULL && crsr.col > e->lines[crsr.line].filled_size)
    {
        crsr.col = e->lines[crsr.line].filled_size;
        if(crsr.col > 0) crsr.col -= 1;
    }
    //else
    //{
    //    crsr.col = 0;
    //}

    // viewport has to fit into Display
    if(!(v->x0 + v->cols <= disp->cols)) return;
    if(!(v->y0 + v->lines <= disp->lines)) return;
    


    // prepare cursors
    // TODO maybe vieportOffset (scolling offset) ist better located in Display instead of Editor
    //scroll to the right if necessary
    if(crsr.col >= v->scrollOffset.cols + v->cols)
    {
        if(v->scrollOffset.cols + v->cols <= e->lines[crsr.line].filled_size)
        {
            v->scrollOffset.cols = crsr.col  - v->cols;
            //v->scrollOffset.cols += 1;
        }
    }
    //scroll to the left if necessary
    if(crsr.col >= 0 && 
       crsr.col < v->scrollOffset.cols)
    {
        v->scrollOffset.cols = crsr.col;
    }
    // scroll down
    if(crsr.line > v->lines-1)
    {
        if(e->count - v->scrollOffset.lines >= v->lines-1)
        {

            v->scrollOffset.lines = crsr.line - v->lines +1;
            //v->scrollOffset.lines +=1;
        }
    }
    // scroll up
    if(crsr.line < v->scrollOffset.lines)
    {

        v->scrollOffset.lines =crsr.line;
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

    int crsr_col = crsr.col - v->scrollOffset.cols; // can get negative?
    disp->crsr.col  = v->x0 + MAX(0,crsr_col);
    disp->crsr.line = crsr.line - v->scrollOffset.lines +v->y0;

}

bool editor_message_check(const Editor *e)
{
    return e->message_count > 0 && e->message;
}

void editor_message_set(Editor *e, const char *s)
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

    if(e->count > e->crsr.line)
    {

        // edit exising line
        #define cur_line (e->lines[e->crsr.line])
        //size_t needed_capcacity = strlen(s) + cur_line.filled_size;
        size_t needed_capcacity = len + cur_line.filled_size;
        if(needed_capcacity < cur_line.total_size)
        {

            
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

            size_t new_total_size = MAX(1, cur_line.total_size * LINE_GROTH_FACTOR);
            while(new_total_size < needed_capcacity) new_total_size *= LINE_GROTH_FACTOR; 
            

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

bool editor_search_next(Editor *e)
{
    assert(e);
    assert(e->search_field);
    const char* s = e->search_field->content;
    size_t len_s = e->search_field->content_count;

    if(!s || len_s == 0) return false;
    // TODO use faster algorithm, Rabin Karp, Boyer-Moore, Knuth-Morris-Pratt etc.
    size_t col = e->crsr.col;
    size_t line = e->crsr.line;
    if(searchfield_same_as_previous_search(e->search_field))
    {
        // avoid finding the same
        col += 1;
    }
    const char *found = NULL;
    bool eof = !(line >= 0 && line < e->count && col >=0);
    while(!(found || eof))
    {
        found = NULL;
        if(col < e->lines[line].filled_size)
        {
            char *ptr = &(e->lines[line].content[col]);
            size_t rem_len = e->lines[line].filled_size - col;
            // strcasestr relies on terminating \0, not usefull here
            //found = strcasestr(ptr, s);
            found = substr_no_case(ptr, rem_len, s, len_s);
        }
        if(!found)
        {
            line += 1;
            col = 0;
        }
        if(line >= e->count)
        {
            eof = true;
        }
    }
    if(found)
    {
        size_t new_col = found - &(e->lines[line].content[0]);
        e->crsr.col = new_col;
        e->crsr.line = line;
        searchfield_remember_previous_search(e->search_field);

        return true;
    }
    else
    {

        return false;
    }
}

bool isstop(char c)
{
    if(isblank(c)) return true;
    static const char stops[] = {';',',','.','\0'};
    for(const char *p=stops; *p; p++)
    {
        if(*p == c) return true; 
    }
    return false;
}

Cursor editor_next_word(Editor *e)
{
    Cursor crsr = e->crsr;
    if(e->count == 0)
    {
        return crsr;
    }
    if(e->count == 1 && e->lines[0].filled_size == 0) {
        return crsr;
    }

    if(crsr.line >= e->count) 
    {
        crsr.line = 0;
        crsr.col = 0;
    }
    // consume until whitespace
    for(;crsr.col < e->lines[crsr.line].filled_size && !isstop(e->lines[crsr.line].content[crsr.col])
        ; crsr.col++);
    // consume all blanks
    for(;crsr.col < e->lines[crsr.line].filled_size && isstop(e->lines[crsr.line].content[crsr.col])
        ; crsr.col++);
    if(crsr.col >= e->lines[crsr.line].filled_size) {
        crsr.col = 0;
        crsr.line += 1; 
        if(crsr.line >= e->count) crsr.line = 0;
        // consume all blanks
        for(;crsr.col < e->lines[crsr.line].filled_size && isblank(e->lines[crsr.line].content[crsr.col])
            ; crsr.col++);
    } 
    return crsr;
}


Cursor editor_prev_word(Editor *e)
{
    // TODO does not go to the beginning of the word sticks in the end
    // TODO does not go to prev line
    Cursor crsr = e->crsr;
    if(e->count == 0)
    {
        return crsr;
    }
    if(e->count == 1 && e->lines[0].filled_size == 0) {
        return crsr;
    }

    if(crsr.line >= e->count)
    {
        crsr.line = e->count;
        if(crsr.line > 0) crsr.line -= 1;
    }
    if(crsr.col >= e->lines[crsr.line].filled_size)
    {
        crsr.col  = e->lines[crsr.line].filled_size;
        if(crsr.col > 0) crsr.col -= 1;
    }

    if(crsr.line == 0 && crsr.col == 0) 
    {
        crsr.line = e->count - 1;
        crsr.col  = e->lines[crsr.line].filled_size;
        if(crsr.col > 0) crsr.col -= 1;
    }

    if(!isstop(e->lines[crsr.line].content[crsr.col]))
    {
        // start in the middle of a word, goto beginning word
        // except when it already was the beginning 

        if(crsr.col > 0)
        {
            crsr.col -= 1;
        }
        else
        {
            if(crsr.line > 0)
            {
                crsr.line -= 1;
            }
            else
            {
                crsr.line = e->count - 1;
            }
            crsr.col = e->lines[crsr.line].filled_size;
            if(crsr.col > 0) crsr.col -= 1;
        }
    }
    // start on a blank
    // consume all blanks
    for(;crsr.col > 0 && isstop(e->lines[crsr.line].content[crsr.col])
        ; crsr.col--);
    if(crsr.col == 0 && isstop(e->lines[crsr.line].content[crsr.col]))
    {
        if(crsr.line > 0)
        {
            crsr.line -= 1;
        }
        else
        {
            crsr.line = e->count -1;
        }
        crsr.col = e->lines[crsr.line].filled_size;
        if(crsr.col > 0) crsr.col -= 1;
    }
    // consume until whitespace reached
    for(;crsr.col > 0 && !isstop(e->lines[crsr.line].content[crsr.col-1])
        ; crsr.col--);

    return crsr;
}