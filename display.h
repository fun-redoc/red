#ifndef __DISPLAY_H__
#define __DISPLAY_H__
#include <stdlib.h>
#include <stdbool.h>
#include "defs.h"
#include "maybe.h"

typedef struct {
    char *viewbuffer;
    size_t lines;
    size_t cols;
    Cursor crsr;
    Scroll scrollOffset;
} Display;

/**
 * @brief initializes display structure, allocates display buffer
 * 
 * @param d 
 * @param lines 
 * @param cols 
 * @return EXIT_FAILURE or EXIT_SUCCESS
 */
int display_init(Display *d, const size_t lines, const size_t cols);

/**
 * @brief cleans up and frees display buffer
 * 
 * @param d 
 */
void display_free(Display *d);

/**
 * @brief risizes (reallocates) display buffer and adjust internal state
 *        handles terminal signals
 * @param d 
 * @return EXIT_FAILURE or EXIT_SUCCESS
 */
int display_resize(Display *d);

/**
 * @brief renders buffer to terminal
 * 
 * @param d 
 */
void display_render_to_terminal(const Display *d);

#endif