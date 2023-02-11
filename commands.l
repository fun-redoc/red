%{
    #include <stdlib.h>
    #include <stdio.h>
    #include <string.h>
    #include <ctype.h>
    #include <assert.h>
    #include <setjmp.h>
    #include "defs.h"
    #include "handler.h"

    typedef enum {LEFT, RIGHT, DOWN, UP, CR} TCommand;
    typedef struct
    {
        char* s;
        size_t len;

    } TStrBuf;

    struct _yylval 
    {
        TCommand cmd;
        int      valNum;
        TStrBuf  valStr;
    } yylval;

    extern jmp_buf try;
    extern Editor e;
    extern Display d;
    extern Viewport editor_viewport;  
    extern Viewport search_viewport;  
    extern Viewport message_viewport; 

    void yyerror(char *s) {
        fprintf(stderr, "YYERR: %s\n", s);
        longjmp(try, EXIT_FAILURE);
    }

#define YY_INPUT(buf,result,max_size) \
    { \
    int c = getchar(); \
    result = (c == EOF) ? YY_NULL : (buf[0] = c, 1); \
    }
%}

%option noyywrap

%option stack
%x browse 
%x insert
%x search

%%
<browse>q                handle_quit(&e, yytext); return 0;
<browse>\x18\x73         handle_save(&e, yytext); return 0;                 /* ctrl-x s - emacs like */
<browse>\n|\r\n          handle_browse_cr(&e, yytext); return 0;
<browse>h|\x1b\x5b\x44   handle_go_left(&e, yytext); return 0;
<browse>l|\x1b\x5b\x43   handle_go_right(&e, yytext); return 0;
<browse>j|\x1b\x5b\x42   handle_go_down(&e, yytext); return 0;
<browse>k|\x1b\x5b\x41   handle_go_up(&e, yytext); return 0;
<browse>\x24|\x05        handle_browse_to_end_of_line(&e, yytext); return 0; /* $ or ctrl-e */
<browse>G                handle_browse_to_end_of_file(&e, yytext); return 0;
<browse>0|\x01           handle_browse_to_begin_of_line(&e, yytext); return 0; /* 0 or ctrl-a */
<browse>gg               handle_browse_to_begin_of_file(&e, yytext); return 0; /* 0 or ctrl-a */
<browse>\x7f             handle_backspace_browse_mode(&e, yytext); return 0;
<browse>d|\x04           handle_delete_browse_mode(&e, yytext); return 0;
<browse>i                yy_push_state(insert); handle_enter_insert_mode(&e, yytext); return 0;

<browse>. {
    fprintf(stderr,"\"");
    for (int i = 0; i < yyleng; ++i) {
        fprintf(stderr,"\\x%02x", yytext[i]);
    }
    fprintf(stderr,"\"\n");
}

<insert>\x1b            yy_pop_state(); handle_leave_insert_mode(&e, yytext); return 0;
<insert>\x7f            handle_backspace_insert_mode(&e, yytext); return 0;
<insert>\x04            handle_delete_insert_mode(&e, yytext); return 0;
<insert>\n|\r\n         handle_insert_cr(&e, yytext); return 0;
<insert>. {
    handle_generic_insertmode(&e, yytext); 
    return 0;
}

%%

void lexer_initialize() {
    yy_push_state(browse);
}

void lexer_finalize() {
    yy_pop_state();
}

/*
handle_enter_search_mode(&e, yytext); return 0;
handle_leave_search_mode(&e, yytext); return 0;
handle_delete_search_mode(&e, yytext); return 0;
handle_left_search_mode(&e, yytext); return 0;
handle_right_search_mode(&e, yytext); return 0;

handle_generic_searchmode(&e, yytext); return 0;
*/

//int yywrap(void) {
//    return 1;
//}

//int main(void)
//{
//    yyin = stdin; 
//    yylex();
//    return 0;
//}