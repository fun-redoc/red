#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "search_field.h"

SearchField *searchfield_init()
{
    SearchField *sf = malloc(sizeof(SearchField));
    sf->crsr = (Cursor){0, 0};
    sf->content_capacity = INITIAL_SEARCH_FIELD_SIZE;
    sf->content = malloc(sizeof(char) * sf->content_capacity);
    sf->content_count = 0;
    return sf;
}
void searchfield_free(SearchField *sf)
{
    if (sf)
    {
        free(sf->content);
        sf->content_capacity = 0;
        sf->content = NULL;
        sf->content_count = 0;
        sf->crsr = (Cursor){0, 0};
    }
}
void searchfield_expand(SearchField *sf)
{
    sf->content_capacity *=2;
    sf->content = realloc(sf->content,sizeof(char)*sf->content_capacity);
}
void searchfield_edit(SearchField *sf, const char *s)
{
    //fprintf(stderr, "TRACE: in searchfield_edit\n");
    //fprintf(stderr, "input=%s\n",s);
    // make sure there is enough space for the entry
    while(sf->crsr.col + strlen(s) > sf->content_capacity )
    {
        searchfield_expand(sf);
    }
    // TODO fill possibly not initialized content buffer with spaces

    // overide mode
    // TODO make it insert mode
    // TODO hadle scroll
    //fprintf(stderr, "s=%s, len(s)=%zu content=%s, crsr.col=%zu, count=%zu, capa=%zu\n", s, strlen(s), sf->content, sf->crsr.col, sf->content_count, sf->content_capacity);
    memcpy(&(sf->content[sf->crsr.col]),s,strlen(s));
    sf->crsr.col += strlen(s);
    sf->content_count = MAX(sf->content_count, sf->crsr.col);
}
void searchfield_render(const SearchField *sf, Viewport *v, Display *d)
{
    //fprintf(stderr, "TRACE: in searchfield_render \n");
    //fprintf(stderr, "TRACE: viewport x0=%zu, y0=%zu, lines=%zu, cols=%zu\n", v->x0, v->y0, v->lines, v->cols);
    //fprintf(stderr, "TRACE: searchfield count=%zu, crsr.line=%zu crsr.col=%zu\n", sf->content_count, sf->crsr.line, sf->crsr.col);
    //fprintf(stderr, "TRACE: display lines=%zu, cols=%zu\n", d->lines, d->cols);
    assert(sf && v && d);
    assert(v->lines == 1);
    // TODO check if viewport fits with other viewport elsewhere
    if(d->cols >= v->x0 + v->cols &&
       d->lines >= (v->y0 + v->lines) && 
       v->cols > strlen(CAPTION_SEARCH))
    {
        //fprintf(stderr, "TRACE: rendering possible\n");
        size_t max_contet_len = MIN(sf->content_count+ strlen(CAPTION_SEARCH), v->cols);
        memset(&(d->viewbuffer[(v->y0)*d->cols+(v->x0)]), ' ', max_contet_len);
        memcpy(&(d->viewbuffer[(v->y0)*d->cols+(v->x0)]), CAPTION_SEARCH, strlen(CAPTION_SEARCH));
        if(sf->content_count > 0) 
        {
            memcpy(&(d->viewbuffer[(v->y0)*d->cols+(v->x0) + strlen(CAPTION_SEARCH)]), sf->content, max_contet_len);
        }
        d->crsr.col  = sf->crsr.col  - v->scrollOffset.cols + v->x0 + strlen(CAPTION_SEARCH);
        d->crsr.line = sf->crsr.line - v->scrollOffset.lines +v->y0;
    }
}

    void searchfield_delete(SearchField *sf)
    {
        fprintf(stderr, "TRACE: searchfield delete\n");
        if(sf->content_count > 0 && sf->crsr.col > 0)
        {
            if(sf->crsr.col > sf->content_count)
            {
                fprintf(stderr, "TRACE: searchfield delete - nothing to delete only coursor move\n");
                sf->crsr.col -= 1;
            }
            else
            {
                fprintf(stderr, "TRACE: searchfield delete - realy deleteing\n");
                size_t l = sf->content_capacity-sf->crsr.col;
                fprintf(stderr, "crsr.col=%zu, amount=%zu\n", sf->crsr.col, l);
                memmove(&(sf->content[sf->crsr.col-1]),&(sf->content[sf->crsr.col]),l);
                sf->content_count -= 1;
                sf->crsr.col -= 1;
            }
        }
    }
    void searchfield_move_left(SearchField *sf)
    {
        if(sf->crsr.col > 0) sf->crsr.col -= 1;
    }
    void searchfield_move_right(SearchField *sf)
    {
        if(sf->crsr.col < sf->content_count) sf->crsr.col += 1;

    }