%{
    #include <stdlib.h>
    #include <stdio.h>
    #include <string.h>
    #include <ctype.h>
    #include <assert.h>
    #include <setjmp.h>
    #include "defs.h"

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
    void yyerror(char *s) {
        fprintf(stderr, "YYERR: %s\n", s);
        longjmp(try, EXIT_FAILURE);
    }
%}

%option noyywrap

%%
\n|\r\n   { 
    yylval.cmd = CR;
}

. {
    yyerror("Unknown character");
}

%%

//static int yywrap(void) {
//    return 1;
//}

//int main(void)
//{
//    yyin = stdin; 
//    yylex();
//    return 0;
//}