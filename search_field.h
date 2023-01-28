#ifndef __SEARCH_FIELD_H__
#define __SEARCH_FIELD_H__

#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "display.h"

#define INITIAL_SEARCH_FIELD_SIZE 1

typedef struct 
{
    Cursor crsr;
    char *content;
    size_t content_capacity;
    size_t content_count;
} SearchField;

SearchField *searchfield_init();
void searchfield_free(SearchField *sf);
void searchfield_edit(SearchField *sf, const char *s);
void searchfield_render(const SearchField *sf, Viewport *v, Display *d);
void searchfield_delete(SearchField *sf);
void searchfield_move_left(SearchField *sf);
void searchfield_move_right(SearchField *sf);

bool searchfield_equal(const SearchField *s1, const SearchField *s2);

#endif
