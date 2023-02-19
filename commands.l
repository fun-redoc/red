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
<browse>gg               handle_browse_to_begin_of_file(&e, yytext); return 0; 
<browse>\x7f             handle_backspace_browse_mode(&e, yytext); return 0;
<browse>dd               handle_delete_line_browse_mode(&e, yytext); return 0;
<browse>x|\x04           handle_delete_browse_mode(&e, yytext); return 0;
<browse>i                yy_push_state(insert); handle_enter_insert_mode(&e, yytext); return 0;
<browse>s|\/             yy_push_state(search); handle_enter_search_mode(&e, yytext); return 0;
<browse>w                handle_next_word(&e, yytext); return 0;
<browse>b                handle_prev_word(&e, yytext); return 0;
<browse>[0-9]+gg {  
    fprintf(stderr, "<browse>[0-9]+gg\n");
    size_t tmp_len  = yyleng - 1;
    char *tmp = calloc((tmp_len), sizeof(char));
    if(tmp)
    {
        fprintf(stderr, "<browse>[0-9]+gg -- 01\n");
        memset(tmp, '\0', tmp_len);
        assert(tmp_len >= yyleng-2);
        strncpy(tmp, yytext, yyleng-2);
        fprintf(stderr, "<browse>[0-9]+gg -- 02 %s %s\n", yytext, tmp);
        int goto_line = atoi(tmp);
        handle_goto_line(&e, goto_line); 
        free(tmp);
        return 0;
    }
}

<browse>. {
    fprintf(stderr,"\"");
    for (int i = 0; i < yyleng; ++i) {
        fprintf(stderr,"\\x%02x", yytext[i]);
    }
    fprintf(stderr,"\"\n");
}

<insert>\x1b           yy_pop_state(); handle_leave_insert_mode(&e, yytext); return 0;
<insert>\x18\x73       handle_save(&e, yytext); return 0;                 /* ctrl-x s - emacs like */
<insert>\x1b\x5b\x44   handle_go_left(&e, yytext); return 0;
<insert>\x1b\x5b\x43   handle_go_right(&e, yytext); return 0;
<insert>\x1b\x5b\x42   handle_go_down(&e, yytext); return 0;
<insert>\x1b\x5b\x41   handle_go_up(&e, yytext); return 0;
<insert>\x05           handle_browse_to_end_of_line(&e, yytext); return 0; /* $ or ctrl-e */
<insert>\x18\x65       handle_browse_to_end_of_file(&e, yytext); return 0; /* ctrl-x e */
<insert>0|\x01         handle_browse_to_begin_of_line(&e, yytext); return 0; /* 0 or ctrl-a */
<insert>\x14           handle_browse_to_begin_of_file(&e, yytext); return 0; /* ctrl-t */
<insert>\x7f           handle_backspace_browse_mode(&e, yytext); return 0;
<insert>\x04           handle_delete_insert_mode(&e, yytext); return 0;
<insert>\n|\r\n        handle_insert_cr(&e, yytext); return 0;
<insert>\x18\x63       handle_quit(&e, yytext); return 0; /* ctrl-x c */
<insert>. {
    fprintf(stderr,"\"");
    for (int i = 0; i < yyleng; ++i) {
        fprintf(stderr,"\\x%02x", yytext[i]);
    }
    fprintf(stderr,"\"\n");
    handle_generic_insertmode(&e, yytext); 
    return 0;
}

<search>\x1b            yy_pop_state(); handle_leave_search_mode(&e, yytext); return 0;
<search>\x7f            handle_delete_search_mode(&e, yytext); return 0;
<search>\x04            handle_delete_search_mode(&e, yytext); return 0;
<search>\x1b\x5b\x44    handle_left_search_mode(&e, yytext); return 0;
<search>\x1b\x5b\x43    handle_right_search_mode(&e, yytext); return 0;
<search>\n|\r\n         yy_pop_state(); handle_search_next(&e, yytext); handle_leave_search_mode(&e, yytext); return 0;
<search>. {
    // TODO schlouldnt i check if yytext is only alnum?
    handle_generic_searchmode(&e, yytext); 
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