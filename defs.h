#ifndef __DEFS_H__
#define __DEFS_H__

// special characters
#define SCROLLED_OUT_OF_SIGHT '#'
#define EPMTY_LINE            '~'
#define NEW_LINE              '\n'
#define BLANK                 '\x20'

// key sequences
#define ENTER     "\r"
#define ESCAPE    "\x1b"
#define BACKSPACE "\x7f"
#define DELETE    "\x1b\x5b\x33\x7e"
#define CTRL_X    "\x18"

// ANSI TERMINAL COMMANDS
#define ANSI_HOME       "\033[H"
#define ANSI_CURSOR_XY "\033[%zu;%zuH"

// Captions (TODO i8n)
#define CAPTION_SEARCH "Search:"
#define MESSAGE_INSERT_MODE "--INSERT--"

#define DO_TRACE true
#define TRACE_TO stdout
#define TRACE_DO(do_this) { if(DO_TRACE) do_this; }

#define LEN(a) (sizeof((a))/sizeof((a[0])))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))


typedef struct {
    size_t line, col;
} Cursor;

typedef struct {
    size_t lines;
    size_t cols;
} Scroll;

typedef struct {
    size_t x0, y0;
    size_t lines, cols;
    Scroll scrollOffset; 
} Viewport;


#endif