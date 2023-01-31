#!/bin/bash
clear
cc search_field.c display.c editor.c main.c -O0 -o red -g -arch x86_64
cc search_field.c display.c editor.c main.c -DUNIT_TEST -O0 -o red-unit -g -arch x86_64
# cc search_field.c display.c editor.c main.c -O2 -o red -arch arm64 -m64