# Get the base Ubuntu image from Docker Hub
#FROM ubuntu:latest
# Get the GCC preinstalled image from Docker Hub
FROM gcc:latest



# Update apps on the base image
RUN apt-get -y update && apt-get install -y

# Install the Clang compiler
RUN apt-get -y install clang

# Install Valgrind
RUN apt-get -y install valgrind

# Copy the current folder which contains C++ source code to the Docker image under /usr/src
COPY . /usr/src/red

# Specify the working directory
WORKDIR /usr/src/red

# Delete executable
RUN rm -f red
RUN rm -Rf red.dSYM

# Use Clang to compile the Test.cpp source file
#RUN clang search_field.c display.c editor.c main.c -g -O0 -o red 
# Use GCC to compile the Test.cpp source file
RUN gcc search_field.c display.c editor.c main.c -g -g3 -ggdb -O0 -o red 


# check for licks
#RUN valgrind --leak-check=yes ./red test.txt

# Run the output program from the previous step
#CMD ["ls", "-a"]
#CMD ["./red", "test.txt"]
CMD [ "valgrind", "--tool=memcheck", "--leak-check=yes", "--show-reachable=yes", "--num-callers=20", "--track-fds=yes", "--track-origins=yes", "./red", "test.txt"]