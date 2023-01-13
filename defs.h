#ifndef __DEFS_H__
#define __DEFS_H__

#define NEW_LINE  '\n'
#define BLANK     '\x20'
#define ESCAPE    "\x1b"
#define BACKSPACE "\x7f"
#define DELETE    "\x1b\x5b\x33\x7e"

#define DO_TRACE true
#define TRACE_TO stdout
#define TRACE_DO(do_this) { if(DO_TRACE) do_this; }

#define LEN(a) (sizeof((a))/sizeof((a[0])))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

#endif