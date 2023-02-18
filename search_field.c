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
    sf->prev_search = NULL;
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
        if(sf->prev_search)
        {
            free(sf->prev_search);
        }
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


bool searchfield_equal(const SearchField *s1, const SearchField *s2)
{
    //bool res = (s1 != NULL) ==> (s2 != NULL);
    bool res = !(s1 != NULL) || (s2 != NULL);
    if(res && s1 && s2) 
    {
        res &= s1->crsr.col == s2->crsr.col && s1->crsr.line == s2->crsr.line;
        //res &= s1->content_capacity == s2->content_capacity;
        res &= s1->content_count == s2->content_count;
        res &= (0 == strncmp(s1->content, s2->content, s1->content_count));
        res &= (s1->prev_search == NULL && s2->prev_search == NULL) ||
               ((s1->prev_search != NULL && s2->prev_search != NULL) && (0 == strcmp(s1->prev_search, s2->prev_search)));
    }
    return res;
}

void searchfield_remember_previous_search(SearchField *sf)
{
    assert(sf);
    if(sf->prev_search) free(sf->prev_search);
    if(sf->content && sf->content_count > 0)
    {
        sf->prev_search = calloc(sf->content_count+1, sizeof(char)); // calloc fills with \0
        strncpy(sf->prev_search, sf->content, sf->content_count);
    }
}

bool searchfield_same_as_previous_search(SearchField *sf)
{
    assert(sf);
    if(!sf->prev_search) return false;
    if(sf->content_count < 1) return false;
    return 0 == strncmp(sf->prev_search, sf->content, sf->content_count);
}