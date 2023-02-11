#!/bin/bash
FILES="lex.yy.c search_field.c display.c editor.c handler.c main.c"
clear
flex commands.l
if [ $? -eq 0 ]; then
    cc $FILES -O0 -o red -g -arch x86_64
fi
if [ $? -eq 0 ]; then
    cc $FILES -DLEX_VERSION -O0 -o red-lex -g -arch x86_64
fi
if [ $? -eq 0 ]; then
    cc $FILES -DUNIT_TEST -O0 -o red-unit -g -arch x86_64
fi
# cc search_field.c display.c editor.c main.c -O2 -o red -arch arm64 -m64