#ifndef __TESTS_C__
#define __TESTS_C__

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
#include <stdbool.h>
#include "maybe.h"
#include "defs.h"
#include "display.h"
#include "editor.h"
    
//jmp_buf try;

int test_frame(Editor *initial, void (*testfn)(Editor *e, const char *s), Editor *expected)
{

    Display d;
    //Editor e;
    Viewport editor_viewport;  
    Viewport search_viewport;  
    Viewport message_viewport; 

    // TEST
    int ret_val = 0;
    char *line = NULL;

    if(EXIT_FAILURE == display_init(&d, 20,20))
    {
        fprintf(stderr, "ERROR: failed to allocate display.\n");
        GOTO_FINISH(1);     
    } 

    editor_viewport  = (Viewport){2,2         ,d.lines-2,d.cols-2, (Scroll){0,0}};
    search_viewport  = (Viewport){0,d.lines-1,1       ,d.cols, (Scroll){0,0}};
    message_viewport = (Viewport){0,d.lines-1,1       ,d.cols, (Scroll){0,0}};

    // using lonjump to be able to handle exceptions from resize handler appropriatelly
    // kind of try-catch with no syntactic sugar
    //int catch_exception = setjmp(try);
    //if(catch_exception==EXIT_FAILURE)
    //{
    //    fprintf(stderr, "ERROR: caught an error\n");
    //    GOTO_FINISH(1);
    //}

        editor_viewport.lines = d.lines-2-1;
        editor_viewport.cols = d.cols-2;
        editor_render(initial, &editor_viewport, &d);

        message_viewport.y0 = d.lines-1;
        message_viewport.cols = d.cols;
        editor_message_render(initial, &message_viewport, &d);

        //e.crsr.line = 1;
        //e.crsr.col = 5;

        // call the function / handler to test
        testfn(initial, NULL);

        editor_render(initial, &editor_viewport, &d);
        editor_message_render(initial, &message_viewport, &d);

        assert(editor_equals(initial, expected));
    
finish:
    if(line) free(line);
    display_free(&d);
    return ret_val;
}

int test_delete_line_if_empty_1()
{

    // TEST
    int ret_val = 0;
    const char * test_file = "test.txt";
    char *fname = malloc(strlen(test_file)+1);
    char *fname1 = malloc(strlen(test_file)+1);

    Editor tested = {
    strcpy(fname, test_file),
    insert,
    NULL,
    0,
    0,
    (Cursor){0,0},
    {0,0},
    NULL,
    NULL,
    0,
    0
    };
    editor_append_line(&tested, "");

    Editor expected = {
    strcpy(fname1, test_file),
    insert,
    NULL,
    0,
    0,
    {0,0},
    {0,0},
    NULL,
    NULL,
    0,
    0
    };

    
    ret_val = test_frame(&tested, handle_delete_insert_mode, &expected);
finish:
    fprintf(stderr, "TRACE: finishing test ret_val==%d\n", ret_val);
    editor_free(&tested);
    editor_free(&expected);
    return ret_val;
}


int test_delete_line_if_empty()
{

    // TEST
    int ret_val = 0;
    //char *line = NULL;
    const char * test_file = "test.txt";
    char *fname = malloc(strlen(test_file)+1);
    char *fname1 = malloc(strlen(test_file)+1);

    Editor tested = {
    strcpy(fname, test_file),
    insert,
    NULL,
    0,
    0,
    (Cursor){0,0},
    {0,0},
    NULL,
    NULL,
    0,
    0
    };

    Editor expected = {
    strcpy(fname1, test_file),
    insert,
    NULL,
    0,
    0,
    {0,0},
    {0,0},
    NULL,
    NULL,
    0,
    0
    };

    ret_val = test_frame(&tested, handle_delete_insert_mode, &expected);
    
finish:
    fprintf(stderr, "TRACE: finishing program ret_val==%d\n", ret_val);
    editor_free(&tested);
    editor_free(&expected);
    return ret_val;
}


int test_delete_empty_line()
{

    // TEST
    int ret_val = 0;
    const char * test_file = "test.txt";
    char *fname = malloc(strlen(test_file)+1);
    char *fname1 = malloc(strlen(test_file)+1);

    Editor tested = {
    strcpy(fname, test_file),
    insert,
    NULL,
    0,
    0,
    (Cursor){1,0},
    {0,0},
    NULL,
    NULL,
    0,
    0
    };
    editor_append_line(&tested, "1.123456789");
    editor_append_line(&tested, "");
    editor_append_line(&tested, "3.123456789");

    Editor expected = {
    strcpy(fname1, test_file),
    insert,
    NULL,
    0,
    0,
    {1,0},
    {0,0},
    NULL,
    NULL,
    0,
    0
    };
    editor_append_line(&expected, "1.123456789");
    editor_append_line(&expected, "3.123456789");

    ret_val = test_frame(&tested, handle_delete_insert_mode, &expected);
    
finish:
    editor_free(&tested);
    editor_free(&expected);
    return ret_val;
}


int test_insert_in_the_middle_of_a_line()
{
    // TEST
    int ret_val = 0;
    const char * test_file = "test.txt";
    char *fname = malloc(strlen(test_file)+1); // one for terminating \0
    char *fname1 = malloc(strlen(test_file)+1);

    Editor tested = {
    strcpy(fname, test_file),
    insert,
    NULL,
    0,
    0,
    {1,6},
    {0,0},
    NULL,
    NULL,
    0,
    0
    };
    editor_append_line(&tested, "1.123456789");
    editor_append_line(&tested, "2.123456789");
    editor_append_line(&tested, "3.123456789");

    Editor expected = {
    strcpy(fname1, test_file),
    insert,
    NULL,
    0,
    0,
    {2,0},
    {0,0},
    NULL,
    NULL,
    0,
    0
    };
    editor_append_line(&expected, "1.123456789");
    editor_append_line(&expected, "2.1234");
    editor_append_line(&expected, "56789");
    editor_append_line(&expected, "3.123456789");

    ret_val = test_frame(&tested, handle_insert_cr, &expected);
finish:
    fprintf(stderr, "TRACE: finishing program ret_val==%d\n", ret_val);
    editor_free(&tested);
    editor_free(&expected);
    return ret_val;
}

int test_backspace_beginn_of_line()
{
    // TEST Scenario
    // 1. cursor is at pos 0 of a nonempty line
    // 2. backspace
    // result should be:
    //  - the line is concatenated with the previous line
    //  - curosr line is decreased, cursor col is position to the end of the previous line
    int ret_val = 0;
    const char * test_file = "test.txt";
    char *fname = malloc(strlen(test_file)+1); // one for terminating \0
    char *fname1 = malloc(strlen(test_file)+1);

    Editor tested = {
    strcpy(fname, test_file),
    insert,
    NULL,
    0,
    0,
    {1,0}, // <- crsrs position
    {0,0},
    NULL,
    NULL,
    0,
    0
    };
    editor_append_line(&tested, "1.123456789");
    editor_append_line(&tested, "2.123456789");
    editor_append_line(&tested, "3.123456789");

    Editor expected = {
    strcpy(fname1, test_file),
    insert,
    NULL,
    0,
    0,
    {0,11},
    {0,0},
    NULL,
    NULL,
    0,
    0
    };
    editor_append_line(&expected, "1.1234567892.123456789");
    // crsr posss here                        ^
    editor_append_line(&expected, "3.123456789");

    ret_val = test_frame(&tested, handle_backspace_insert_mode, &expected);
finish:
    fprintf(stderr, "TRACE: finishing program ret_val==%d\n", ret_val);
    editor_free(&tested);
    editor_free(&expected);
    return ret_val;
}

#endif