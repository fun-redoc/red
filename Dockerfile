# Get the base Ubuntu image from Docker Hub
#FROM ubuntu:latest
# Get the GCC preinstalled image from Docker Hub
FROM gcc:latest

# Update apps on the base image
RUN apt-get -y update && apt-get install -y
RUN apt-get -y upgrade

# Install the Clang compiler
RUN apt-get -y install clang

# Install Valgrind
RUN apt-get -y install valgrind

# Copy the current folder which contains C++ source code to the Docker image under /usr/src
RUN rm -Rf /usr/src/red
COPY . /usr/src/red

# Specify the working directory
WORKDIR /usr/src/red

# Delete executable
RUN rm -f red
RUN rm -f red-unit
RUN rm -Rf red.dSYM

# Use Clang to compile the Test.cpp source file
RUN clang search_field.c display.c editor.c handler.c main.c -DUNIT_TEST -g -O0
# Use GCC to compile the Test.cpp source file
#RUN gcc search_field.c display.c editor.c main.c -DUNIT_TEST -g -ggdb -O0


# check for licks
#RUN valgrind --leak-check=yes ./red test.txt

# Run the output program from the previous step
#CMD ["ls", "-a"]
#CMD ["./red", "test.txt"]
CMD [ "valgrind", "-v", "-s", "--tool=memcheck", "--leak-check=full", "--show-reachable=yes", "--num-callers=20", "--track-fds=yes", "--track-origins=yes", "./a.out"]
#CMD [ "valgrind", "--leak-check=full", "./a.out"]