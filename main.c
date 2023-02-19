// WORKING ON:

// BACKLOG:
// show information e.g. (col, row) in a status line
// undo
// show message when saved, hide message on following action
// append to Line
// append new line
// when opening with not existing file name create file
// move cursor by paragraph
// warn when quit without saving edited text
// bevore saving check if the file was changed in between
// maybe Bug when row is empty (shows only #, which is but the sign for overscrolling a line aut of visible area)
// : jump to the end or the beginning of line
// insert at position
// show line numbers 
// move cursor with number like vi
// make keybinding configurable via config file (JSON)

// READY
// move cursor by word
// message --NOT FOUND-- when not found
// search and jump to next search position
// goto line number
// delete not empty line
// backspace at crsr.col = 0 (append to prev line)
// BUG goto end of last line, enter insert mode, enter CR, edit -> editor stalls
// commandline for unit test mode or better another destination
// delete empty line
// append line  implement     {insert, "\x0a", handle_insert_cr},
// insert mode: backspace
// insert mode: delete
// saving on Ctrl-s
// fix scroll and movement
// fix abort in finish
// fix resize in (tmux?)
// insert mode: fix move corsor behind the last char and insert at the end

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
#include <setjmp.h>
#include "maybe.h"
#include "defs.h"
#include "display.h"
#include "editor.h"
#include "handler.h"


#define MAX_ESC_SEQ_LEN 255

#define INITIAL_STRING_BUFFER_SIZE 1
#define GOTO_FINISH(_ret_val) { ret_val = (_ret_val); goto finish; }

#ifdef LEX_VERSION
extern FILE *yyin;
extern int  yylex(void);
extern void lexer_initialize();
extern void lexer_finalize();
#endif

// TODO 
//typedef struct
//{
//    Display d;
//    Editor **es;
//    Editor *active_editor;
//    size_t count;
//    size_t capacity;
//} Red;


/*
 * global variables
 */
jmp_buf try;
Display d;
Editor e;
Viewport editor_viewport;  
Viewport search_viewport;  
Viewport message_viewport; 

// helper function to get escape sequence by pressing keys
int printt_escape_seq(void)
{
    char seq[MAX_ESC_SEQ_LEN];
    memset(seq, 0, sizeof(seq));
    int ret = read(STDIN_FILENO, seq, sizeof(seq));
    if (ret < 0) {
        fprintf(stderr, "ERROR: something went wrong during reading user input: %s\n", strerror(errno));
        return 1;
    }
    assert(ret < 32);
    printf("\"");
    for (int i = 0; i < ret; ++i) {
        printf("\\x%02x", seq[i]);
    }
    printf("\"\n");
    return 0;
}

//void handle_resize_signal(int signal)
void handle_resize_signal()
{
    rerender_all(&d, &e, &editor_viewport, &search_viewport, &message_viewport);
}

KeyHandler handler_map[] = {
    {browse, "q", handle_quit},
    {browse, "l", handle_go_right},
    {browse, "h", handle_go_left},
    {browse, "j", handle_go_down},
    {browse, "k", handle_go_up},
    {browse, "\x1b\x5b\x44", handle_go_left},
    {browse, "\x1b\x5b\x43", handle_go_right},
    {browse, "\x1b\x5b\x41", handle_go_up},
    {browse, "\x1b\x5b\x42", handle_go_down},
    {browse, "\x7f", handle_backspace_browse_mode},
    {browse, "\x04", handle_delete_browse_mode},
    {browse, "\x18\x73", handle_save},
    {browse, "\x0a", handle_browse_cr},
    {browse, "$", handle_browse_to_end_of_line},
    {browse, "0", handle_browse_to_begin_of_line},
    {browse, "G", handle_browse_to_end_of_file},
//        {browse, "\x5e", handle_browse_to_first_non_blank},
    {browse, "s", handle_enter_search_mode},
    {browse, "i", handle_enter_insert_mode},
    {insert, "\x1b\x5b\x44", handle_go_left},
    {insert, "\x1b\x5b\x43", handle_go_right},
    {insert, "\x1b\x5b\x41", handle_go_up},
    {insert, "\x1b\x5b\x42", handle_go_down},
    {insert, "\x18\x73", handle_save},
    {insert, "\x7f", handle_backspace_insert_mode},
    {insert, "\x04", handle_delete_insert_mode},
    {insert, ESCAPE, handle_leave_insert_mode},
    {insert, "\x0a", handle_insert_cr},
    {search, ESCAPE, handle_leave_search_mode},
    {search, "\x7f", handle_delete_search_mode},
    {search, "\x04", handle_delete_search_mode},
    {search, "\x1b\x5b\x44", handle_left_search_mode},
    {search, "\x1b\x5b\x43", handle_right_search_mode},
};


int real_main(int argc, char* argv[])
{
    int ret_val = 0;
    char *line = NULL;

    assert(argc == 2 && "ERROR: you have to provide a file name");
    if(!file_exists(argv[1])) 
    {
        fprintf(stderr, "ERROR: file does not exists %s.\n", strerror(errno));
        GOTO_FINISH(1);
    }

    MAYBE(size_t) size = file_size(argv[1]);
    if(IS_NOTHING2(size))
    {
        fprintf(stderr, "ERROR cannot determin file size %s.\n", strerror(errno));
        GOTO_FINISH(1);
    }
    if(MAYBE_VALUE_ACCESS(size) == 0)
    {
        fprintf(stderr, "ERROR File is empty.\n");
        GOTO_FINISH(1);
    }

    if(EXIT_FAILURE == editor_init(&e)) 
    {
        fprintf(stderr, "ERROR: failed to allocate editor.\n");
        GOTO_FINISH(1);     
    } 

    FILE* f = fopen(argv[1], "rb");
    if(!f)
    {
        fprintf(stderr, "ERROR: failed to open filed %s. Reaseon: %s.\n", argv[1], strerror(errno));
        GOTO_FINISH(1);
    }

    size_t line_len = 80;
    line = malloc(sizeof(char)*line_len);
    while(!feof(f))
    {
        ssize_t ret = getline(&line, &line_len, f);
        if(ret == -1 && errno != 0)
        {
            fprintf(stderr, "ERROR: failed to read file: %d - %s.\n", errno, strerror(errno));
            GOTO_FINISH(1);
        }
        if(ret != -1)
        {
            if(!editor_append_line(&e, line))
            {
                fprintf(stderr, "ERROR: failed to load file.\n");
                GOTO_FINISH(1);
            }
        }
    }

    // close the file
    if(f)
    {
        fclose(f);
        f = NULL;
    }

    if(!(e.filepath = malloc(sizeof(char)*strlen(argv[1]))))
    {
        fprintf(stderr, "ERROR: failed to allocate file path.\n");
        GOTO_FINISH(1);
    }
    memcpy(e.filepath, argv[1],strlen(argv[1]));

    // TODO get Terminal size to initialise display size
    if(EXIT_FAILURE == display_init(&d, 20,20))
    {
        fprintf(stderr, "ERROR: failed to allocate display.\n");
        GOTO_FINISH(1);     
    } 

    bool terminal_ready = false;

    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) 
    {
        fprintf(stderr, "ERROR: Program has to be started in real terminal!\n");
        GOTO_FINISH(1);
    }
    struct termios term;
    if (tcgetattr(STDIN_FILENO, &term) < 0) 
    {
        fprintf(stderr, "ERROR: could not determin the state of the term: %s\n", strerror(errno));
        GOTO_FINISH(1);
    }

    term.c_lflag &= ~ECHO;
    term.c_lflag &= ~ICANON;
    if (tcsetattr(0, 0, &term)) 
    {
        fprintf(stderr, "ERROR: could not update the state of the terminal: %s\n", strerror(errno));
        GOTO_FINISH(1);
    }

    terminal_ready = true;

    struct sigaction act, old = {0};
    act.sa_handler = handle_resize_signal;
    if (sigaction(SIGWINCH, &act, &old) < 0) {
        fprintf(stderr, "ERROR: could not set resize handler: %s\n", strerror(errno));
        GOTO_FINISH(1);
    }


    editor_viewport  = (Viewport){2,2         ,d.lines-2,d.cols-2, (Scroll){0,0}};
    search_viewport  = (Viewport){0,d.lines-1,1       ,d.cols, (Scroll){0,0}};
    message_viewport = (Viewport){0,d.lines-1,1       ,d.cols, (Scroll){0,0}};

    // using lonjump to be able to handle exceptions from resize handler appropriatelly
    // kind of try-catch with no syntactic sugar
    int catch_exception = setjmp(try);
    if(catch_exception==EXIT_FAILURE)
    {
        fprintf(stderr, "ERROR: caught an error\n");
        GOTO_FINISH(1);
    }
    while(e.mode != quit) 
    {
        //uncomment to see ascii sequence while typing
        //if((ret_val=printt_escape_seq()) != 0) GOTO_FINISH(ret_val)
        //else continue;

        rerender_all(&d, &e, &editor_viewport, &search_viewport, &message_viewport);

        char seq[MAX_ESC_SEQ_LEN] = {0};
        errno = 0;
        int seq_len = 0;
        seq_len += read(STDIN_FILENO, &(seq[seq_len]), sizeof(seq));
        if (errno == EINTR) 
        {
            // there was a signal from resize not an entry
            // resize is handled in the resize handler completly
            // so ignore this here and loop
            continue;
        }
        if (errno > 0) 
        {
            fprintf(stderr, "ERROR: error while reading input: %s\n", strerror(errno));
            GOTO_FINISH(1);
        }
        assert((size_t) seq_len < sizeof(seq));
        if(strcmp(seq,CTRL_X) == 0)
        {
            seq_len += read(STDIN_FILENO, &(seq[seq_len]), sizeof(seq));
            if (errno == EINTR) 
            {
                // there was a signal from resize not an entry
                // resize is handled in the resize handler completly
                // so ignore this here and loop
                continue;
            }
            if (errno > 0) 
            {
                fprintf(stderr, "ERROR: error while reading input: %s\n", strerror(errno));
                GOTO_FINISH(1);
            }
            assert((size_t) seq_len < sizeof(seq));
            fprintf(stderr, "TRACE: ctrl-x seq: ");
            for (int i = 0; i < seq_len; ++i) {
                fprintf(stderr,"\\x%02x", seq[i]);
            }
            fprintf(stderr, "\"\n");
        }

        void (*key_handler)(Editor*, const char*) = NULL;
        for(size_t i=0; i < LEN(handler_map); i++)
        {
            if(   handler_map[i].mode == e.mode
               && strcmp(seq, handler_map[i].seq) == 0)
            {
                key_handler = handler_map[i].handler;
            }
        }
        if(key_handler) {
            key_handler(&e, seq);
        }
        else
        {
            // no special handler found try gegenric
            bool ok = true;
            switch (e.mode)
            {
            case search:
                for(size_t i=0; i<strlen(seq); i++) ok &= isprint(seq[i]);
                if(ok) handle_generic_searchmode(&e, seq);
                break;
            case insert:
                for(size_t i=0; i<strlen(seq); i++) ok &= isprint(seq[i]);
                if(ok) handle_generic_insertmode(&e, seq);
                break;
            default:
                break;
            }
        }
    }
    
finish:
    fprintf(stderr, "TRACE: result from setjmp %s\n", catch_exception==EXIT_FAILURE? "EXIT_FAILURE": "EXIT_SUCCESS");
    fprintf(stderr, "TRACE: editor mode==%s\n", e.mode==quit? "quit": "not quit");
    fprintf(stderr, "TRACE: finishing program ret_val==%d\n", ret_val);
    if (terminal_ready) 
    {
        // reset treminal 
        printf("\033[2J");
        term.c_lflag |= ECHO;
        term.c_lflag |= ICANON;
        tcsetattr(STDIN_FILENO, 0, &term);
    }
    if(line) free(line);
    editor_free(&e);
    display_free(&d);
    if(f) fclose(f);
    return ret_val;
}


#ifdef LEX_VERSION
int lex_main(int argc, char *argv[])
{
    // TODO code dupplication with real_main
    int ret_val = 0;
    char *line = NULL;

    assert(argc == 2 && "ERROR: you have to provide a file name");
    if(!file_exists(argv[1])) 
    {
        fprintf(stderr, "ERROR: file does not exists %s.\n", strerror(errno));
        GOTO_FINISH(1);
    }

    MAYBE(size_t) size = file_size(argv[1]);
    if(IS_NOTHING2(size))
    {
        fprintf(stderr, "ERROR cannot determin file size %s.\n", strerror(errno));
        GOTO_FINISH(1);
    }
    if(MAYBE_VALUE_ACCESS(size) == 0)
    {
        fprintf(stderr, "ERROR File is empty.\n");
        GOTO_FINISH(1);
    }

    if(EXIT_FAILURE == editor_init(&e)) 
    {
        fprintf(stderr, "ERROR: failed to allocate editor.\n");
        GOTO_FINISH(1);     
    } 

    FILE* f = fopen(argv[1], "rb");
    if(!f)
    {
        fprintf(stderr, "ERROR: failed to open filed %s. Reaseon: %s.\n", argv[1], strerror(errno));
        GOTO_FINISH(1);
    }

    size_t line_len = 80;
    line = malloc(sizeof(char)*line_len);
    while(!feof(f))
    {
        ssize_t ret = getline(&line, &line_len, f);
        if(ret == -1 && errno != 0)
        {
            fprintf(stderr, "ERROR: failed to read file: %d - %s.\n", errno, strerror(errno));
            GOTO_FINISH(1);
        }
        if(ret != -1)
        {
            if(!editor_append_line(&e, line))
            {
                fprintf(stderr, "ERROR: failed to load file.\n");
                GOTO_FINISH(1);
            }
        }
    }

    // close the file
    if(f)
    {
        fclose(f);
        f = NULL;
    }

    if(!(e.filepath = malloc(sizeof(char)*strlen(argv[1]))))
    {
        fprintf(stderr, "ERROR: failed to allocate file path.\n");
        GOTO_FINISH(1);
    }
    memcpy(e.filepath, argv[1],strlen(argv[1]));

    // TODO get Terminal size to initialise display size
    if(EXIT_FAILURE == display_init(&d, 20,20))
    {
        fprintf(stderr, "ERROR: failed to allocate display.\n");
        GOTO_FINISH(1);     
    } 

    bool terminal_ready = false;

    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) 
    {
        fprintf(stderr, "ERROR: Program has to be started in real terminal!\n");
        GOTO_FINISH(1);
    }
    struct termios term;
    if (tcgetattr(STDIN_FILENO, &term) < 0) 
    {
        fprintf(stderr, "ERROR: could not determin the state of the term: %s\n", strerror(errno));
        GOTO_FINISH(1);
    }

    term.c_lflag &= ~ECHO;
    term.c_lflag &= ~ICANON;
    if (tcsetattr(0, 0, &term)) 
    {
        fprintf(stderr, "ERROR: could not update the state of the terminal: %s\n", strerror(errno));
        GOTO_FINISH(1);
    }

    terminal_ready = true;

    struct sigaction act, old = {0};
    act.sa_handler = handle_resize_signal;
    if (sigaction(SIGWINCH, &act, &old) < 0) {
        fprintf(stderr, "ERROR: could not set resize handler: %s\n", strerror(errno));
        GOTO_FINISH(1);
    }


    editor_viewport  = (Viewport){2,2         ,d.lines-2,d.cols-2, (Scroll){0,0}};
    search_viewport  = (Viewport){0,d.lines-1,1       ,d.cols, (Scroll){0,0}};
    message_viewport = (Viewport){0,d.lines-1,1       ,d.cols, (Scroll){0,0}};

    // using lonjump to be able to handle exceptions from resize handler appropriatelly
    // kind of try-catch with no syntactic sugar
    int catch_exception = setjmp(try);
    if(catch_exception==EXIT_FAILURE)
    {
        fprintf(stderr, "ERROR: caught an error\n");
        GOTO_FINISH(1);
    }
    rerender_all(&d, &e, &editor_viewport, &search_viewport, &message_viewport);
    yyin = stdin; 
    lexer_initialize();
    while(e.mode != quit) 
    {
        yylex();
        rerender_all(&d, &e, &editor_viewport, &search_viewport, &message_viewport);
    }
finish:
    lexer_finalize();
    fprintf(stderr, "TRACE: result from setjmp %s\n", catch_exception==EXIT_FAILURE? "EXIT_FAILURE": "EXIT_SUCCESS");
    fprintf(stderr, "TRACE: editor mode==%s\n", e.mode==quit? "quit": "not quit");
    fprintf(stderr, "TRACE: finishing program ret_val==%d\n", ret_val);
    if (terminal_ready) 
    {
        // reset treminal 
        printf("\033[2J");
        term.c_lflag |= ECHO;
        term.c_lflag |= ICANON;
        tcsetattr(STDIN_FILENO, 0, &term);
    }
    if(line) free(line);
    editor_free(&e);
    display_free(&d);
    if(f) fclose(f);
    return ret_val;
}
#endif //LEX_VERSION

#ifdef UNIT_TEST
#include "tests.c"
    int main(int argc, char *argv[])
    {
        int catch_exception = setjmp(try);
        if(catch_exception==EXIT_FAILURE)
        {
            fprintf(stderr, "ERROR: caught an error\n");
            goto finish;
        }
        int res = 0;
        res |= test_insert_in_the_middle_of_a_line();
        res |= test_delete_empty_line();
        res |= test_delete_line_if_empty();
        res |= test_delete_line_if_empty_1();
        res |= test_backspace_beginn_of_line();
        res |= test_delete_line();
        res |= test_insert_behind_capacity();
        res |= test_search_next();
        res |= test_prev_word();
        if(!res) fprintf(stderr, "all tests passed\n");
    finish:
        if(res) fprintf(stderr, "tests not passed\n");
        return res;
    }
#else
    int main(int argc, char *argv[])
    {
        #ifdef LEX_VERSION
            return lex_main(argc, argv);
        #else
            return real_main(argc, argv);
        #endif
    }
#endif