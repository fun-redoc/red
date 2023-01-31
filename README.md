# RED my own editor

__work is in progress, not yet ready, maybe never near to getting usful__

once in his or her life every programer has to (my subjective pick):
+ code (imperfect) clones of all the heritage gameslike pong, moonlander, space invaders, tetris, packman 
+ code an ANN from scratch
+ code all the algorithms from the art of computer programing in his favourite programing language
+ create and implement own programing language
+ code his own code editor
+ code his own 2d/3d graphics library
+ code his own haiku or limerick generator
+ more to come from quantum computing

I'm getting older now, has to hurry up, writing code editor.


## Features
- display file
- resize window
- move cursor
- scroll text when viewport is to small

## Planed Features
- search and repeat search
- goto line number
- search and replace, single and bulk
- insert, change, replace, append, delete
- undo
- save
- copy
- move 
- mark (chars, lines)
- change, delete mark

## More Ideas
- bookmarks
- execute command line, and bind command line scripts to a key shortcut
- compile shorcut, mark compiler result in editor, jump to errors
- compile & test im background 
- edit multpile files 
- copy paste among multiple files
- multi Cursor editing
- block editing
- gdb mode debugging
- connect to laguage servers

## Key Bindings

you can see and adjust all key bindings in the array ```KeyHandler handler_map[]```

### Browse Mode
#### General
- ```q```: quit
#### Movement (like vi)
- ```h```: left
- ```l```: righ
- ```j```: down
- ```k```: up


## Build, Test etc.

build with ```./build.sh```.
start with ```./red <file>```.

There are some tests commented out in main(). Maybe later i will implement testmode.

In testmode one can use venguard to check for memory leaks. i use docker container (see ```Dockerfile```).

Build container with
```docker build --pull --rm -f "Dockerfile" -t red:latest```.

Run Vanguard in docker with
```docker run --rm -it  red:latest```.
