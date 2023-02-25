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

int test_frame(Editor *initial, void (*testfn)(Editor *e, const char *s), const char *s, Editor *expected)
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
        testfn(initial, s);

        editor_render(initial, &editor_viewport, &d);
        editor_message_render(initial, &message_viewport, &d);

        //assert(editor_equals(initial, expected));
        ret_val = !editor_equals(initial, expected);
    
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

    
    ret_val = test_frame(&tested, handle_delete_insert_mode, NULL, &expected);
finish:

    editor_free(&tested);
    editor_free(&expected);
    assert(ret_val == 0);
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

    ret_val = test_frame(&tested, handle_delete_insert_mode, NULL,  &expected);
    
finish:

    editor_free(&tested);
    editor_free(&expected);
    assert(ret_val == 0);
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

    ret_val = test_frame(&tested, handle_delete_insert_mode, NULL, &expected);
    
finish:
    editor_free(&tested);
    editor_free(&expected);
    assert(ret_val == 0);
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

    ret_val = test_frame(&tested, handle_insert_cr, NULL, &expected);
finish:

    editor_free(&tested);
    editor_free(&expected);
    assert(ret_val == 0);
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

    ret_val = test_frame(&tested, handle_backspace_insert_mode, NULL, &expected);
finish:

    editor_free(&tested);
    editor_free(&expected);
    assert(ret_val == 0);
    return ret_val;
}

int test_delete_line()
{
    // TEST Scenario
    // 1. cursor is at pos 3 of a nonempty line
    // 2. delete_line
    // result should be:
    //  - same with the line to delete less
    //  - curosr line is decreased, cursor col is 0
    int ret_val = 0;
    const char * test_file = "test.txt";
    char *fname = malloc(strlen(test_file)+1); // one for terminating \0
    char *fname1 = malloc(strlen(test_file)+1);

    Editor tested = {
    strcpy(fname, test_file),
    browse,
    NULL,
    0,
    0,
    {1,4}, // <- crsrs position
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
    browse,
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
    editor_append_line(&expected, "1.123456789");
    editor_append_line(&expected, "3.123456789");

    ret_val = test_frame(&tested, handle_delete_line_browse_mode, NULL, &expected);
finish:

    editor_free(&tested);
    editor_free(&expected);
    assert(ret_val == 0);
    return ret_val;
}

int test_insert_behind_capacity()
{
    // TEST Scenario
    // 1. cursor is at pos 3 of a nonempty line
    // 2. delete_line
    // result should be:
    //  - same with the line to delete less
    //  - curosr line is decreased, cursor col is 0
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
    {4,4}, // <- crsrs position
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
    {4,0},
    {0,0},
    NULL,
    NULL,
    0,
    0
    };
    editor_append_line(&expected, "1.123456789");
    editor_append_line(&expected, "2.123456789");
    editor_append_line(&expected, "3.123456789");
    editor_append_line(&expected, "");
    editor_append_line(&expected, "hello");

    ret_val = test_frame(&tested, handle_generic_insertmode, "hello", &expected);
finish:

    editor_free(&tested);
    editor_free(&expected);
    assert(ret_val == 0);
    return ret_val;
}

int test_search_next()
{
    // TEST Scenario
    int ret_val = 0;
    const char * test_file = "test.txt";
    char *fname = malloc(strlen(test_file)+1); // one for terminating \0
    char *fname1 = malloc(strlen(test_file)+1);

    SearchField *sf_test = searchfield_init();
    searchfield_edit(sf_test, "hallo");

    Editor tested = {
    strcpy(fname, test_file),
    search,
    NULL,
    0,
    0,
    {0,3}, // <- crsrs position
    {0,0},
    sf_test,
    NULL,
    0,
    0
    };
    editor_append_line(&tested, "1.123456789");
    editor_append_line(&tested, "2.123456789 Hallo");
    editor_append_line(&tested, "hallo3.123456789");
    editor_append_line(&tested, "4.123hallo456hallo789");

    SearchField *sf_expect = searchfield_init();
    searchfield_edit(sf_expect, "hallo");
    searchfield_remember_previous_search(sf_expect);

    Editor expected = {
    strcpy(fname1, test_file),
    search,
    NULL,
    0,
    0,
    {1,12},
    {0,0},
    sf_expect,
    NULL,
    0,
    0
    };
    editor_append_line(&expected, "1.123456789");
    editor_append_line(&expected, "2.123456789 Hallo");
    editor_append_line(&expected, "hallo3.123456789");
    editor_append_line(&expected, "4.123hallo456hallo789");

    ret_val = test_frame(&tested, handle_search_next, "HALLO", &expected);
    assert(ret_val == 0);
    tested.crsr.col +=1;
    expected.crsr.col = 0;
    expected.crsr.line = 2;
    ret_val |= test_frame(&tested, handle_search_next, "hallo", &expected);
    assert(ret_val == 0);
    tested.crsr.col +=1;
    expected.crsr.col = 5;
    expected.crsr.line = 3;
    ret_val |= test_frame(&tested, handle_search_next, "hallo", &expected);
    assert(ret_val == 0);
    tested.crsr.col +=1;
    expected.crsr.col = 13;
    expected.crsr.line = 3;
    ret_val |= test_frame(&tested, handle_search_next, "hallo", &expected);
    assert(ret_val == 0);
    //tested.crsr.col +=1;
    //expected.crsr.col = 12;
    //expected.crsr.line = 0;
    //ret_val |= test_frame(&tested, handle_search_next, "hallo", &expected);
finish:

    editor_free(&tested);
    editor_free(&expected);
    assert(ret_val == 0);
    return ret_val;
}

int test_prev_word()
{
    // TEST Scenario
    int ret_val = 0;
    const char * test_file = "test.txt";
    char *fname = malloc(strlen(test_file)+1); // one for terminating \0
    char *fname1 = malloc(strlen(test_file)+1);

    Editor tested = {
    strcpy(fname, test_file),
    search,
    NULL,
    0,
    0,
    {123,234}, // <- crsrs position
    {0,0},
    NULL,
    NULL,
    0,
    0
    };
    //                           00000000011111111112
    //                           12345678901234567890
    editor_append_line(&tested, "1. hallo hallo");
    editor_append_line(&tested, "2. Foo Bar; Hallo");
    editor_append_line(&tested, "");
    editor_append_line(&tested, "");
    editor_append_line(&tested, "");
    editor_append_line(&tested, "");
    editor_append_line(&tested, "8. Foo Bar; Hallo");
    //                           00000000011111111112
    //                           12345678901234567890

    Editor expected = {
    strcpy(fname1, test_file),
    search,
    NULL,
    0,
    0,
    {6,12},
    {0,0},
    NULL,
    NULL,
    0,
    0
    };
    //                             00000000011111111112
    //                             12345678901234567890
    editor_append_line(&expected, "1. hallo hallo");
    editor_append_line(&expected, "2. Foo Bar; Hallo");
    editor_append_line(&expected, "");
    editor_append_line(&expected, "");
    editor_append_line(&expected, "");
    editor_append_line(&expected, "");
    editor_append_line(&expected, "8. Foo Bar; Hallo");
    //                             00000000011111111112
    //                             12345678901234567890

    ret_val = test_frame(&tested, handle_prev_word, NULL, &expected);
    assert(ret_val == 0);
    expected.crsr.col = 7;
    ret_val = test_frame(&tested, handle_prev_word, NULL, &expected);
    assert(ret_val == 0);
    expected.crsr.col = 3;
    ret_val = test_frame(&tested, handle_prev_word, NULL, &expected);
    assert(ret_val == 0);
    expected.crsr.col = 0;
    ret_val = test_frame(&tested, handle_prev_word, NULL, &expected);
    assert(ret_val == 0);
    expected.crsr.col = 0;
    expected.crsr.line -= 1;
    ret_val = test_frame(&tested, handle_prev_word, NULL, &expected);
    assert(ret_val == 0);
    expected.crsr.col = 0;
    expected.crsr.line -= 1;
    ret_val = test_frame(&tested, handle_prev_word, NULL, &expected);
    assert(ret_val == 0);
    expected.crsr.col = 0;
    expected.crsr.line -= 1;
    ret_val = test_frame(&tested, handle_prev_word, NULL, &expected);
    assert(ret_val == 0);
    expected.crsr.col = 0;
    expected.crsr.line -= 1;
    ret_val = test_frame(&tested, handle_prev_word, NULL, &expected);
    assert(ret_val == 0);
    expected.crsr.col = 12;
    expected.crsr.line -= 1;
    ret_val = test_frame(&tested, handle_prev_word, NULL, &expected);
    assert(ret_val == 0);
    expected.crsr.col = 7;
    expected.crsr.line -= 0;
    ret_val = test_frame(&tested, handle_prev_word, NULL, &expected);
    assert(ret_val == 0);
    expected.crsr.col = 3;
    expected.crsr.line -= 0;
    ret_val = test_frame(&tested, handle_prev_word, NULL, &expected);
    assert(ret_val == 0);
    expected.crsr.col = 0;
    expected.crsr.line -= 0;
    ret_val = test_frame(&tested, handle_prev_word, NULL, &expected);
    assert(ret_val == 0);
    expected.crsr.col = 9;
    expected.crsr.line -= 1;
    ret_val = test_frame(&tested, handle_prev_word, NULL, &expected);
    assert(ret_val == 0);
    expected.crsr.col = 3;
    expected.crsr.line -= 0;
    ret_val = test_frame(&tested, handle_prev_word, NULL, &expected);
    assert(ret_val == 0);
    expected.crsr.col = 0;
    expected.crsr.line -= 0;
    ret_val = test_frame(&tested, handle_prev_word, NULL, &expected);
    assert(ret_val == 0);
    expected.crsr.col = 12;
    expected.crsr.line = 6;
    ret_val = test_frame(&tested, handle_prev_word, NULL, &expected);
    assert(ret_val == 0);
finish:

    editor_free(&tested);
    editor_free(&expected);
    assert(ret_val == 0);
    return ret_val;
}

int test_go_right()
{
    int ret_val = 0;
    const char * test_file = "test.txt";
    char *fname = malloc(strlen(test_file)+1); // one for terminating \0
    char *fname1 = malloc(strlen(test_file)+1);

    Editor tested = {
    strcpy(fname, test_file),
    browse,
    NULL,
    0,
    0,
    {0,0}, // <- crsrs position
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
    browse,
    NULL,
    0,
    0,
    {0,1},
    {0,0},
    NULL,
    NULL,
    0,
    0
    };
    editor_append_line(&expected, "1.123456789");
    editor_append_line(&expected, "2.123456789");
    editor_append_line(&expected, "3.123456789");

    ret_val = test_frame(&tested, handle_go_right, NULL, &expected);
finish:

    editor_free(&tested);
    editor_free(&expected);
    assert(ret_val == 0);
    return ret_val;
}

int test_go_to_begin_of_line()
{
    int ret_val = 0;
    const char * test_file = "test.txt";
    char *fname = malloc(strlen(test_file)+1); // one for terminating \0
    char *fname1 = malloc(strlen(test_file)+1);

    Editor tested = {
    strcpy(fname, test_file),
    browse,
    NULL,
    0,
    0,
    {0,10}, // <- crsrs position
    {0,0},
    NULL,
    NULL,
    0,
    0
    };
    editor_append_line(&tested, "123456789");
    editor_append_line(&tested, "123456789");
    editor_append_line(&tested, "123456789");

    Editor expected = {
    strcpy(fname1, test_file),
    browse,
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
    editor_append_line(&expected, "123456789");
    editor_append_line(&expected, "123456789");
    editor_append_line(&expected, "123456789");

    ret_val = test_frame(&tested, handle_browse_to_begin_of_line, NULL, &expected);
finish:

    editor_free(&tested);
    editor_free(&expected);
    assert(ret_val == 0);
    return ret_val;
}

#endif