
# Define your c compiler.  I recommend gcc if you've got it.
#CC = cc
CC = gcc

# Define your optimization flags.  Most compilers understand -O and -O2,
# Debugging (don't use)
#OPTS = -Wall -g
# Standard
OPTS = -O2
# Pentium with gcc 2.7.0 or better
#OPTS = -O2 -fomit-frame-pointer -malign-functions=2 -malign-loops=2 \
-malign-jumps=2

# There are a few defines which may be necessary to make frotz compile on
# your system.  Most of these are fairly straightforward.
#    -DNO_MEMMOVE:    Use this if your not-so-standard c library lacks the
#                     memmove(3) function. (SunOS, other old BSDish systems)
#
# You may need to define one of these to get the getopt prototypes:
#    -DUSE_UNISTD_H:  Use this if your getopt prototypes are in unistd.h.
#                     (Solaris?)
#    -DUSE_GETOPT_H:  Use this if you have a separate /usr/include/getopt.h
#                     file containing the getopt prototypes. (Linux, IRIX 5?)
#    -DUSE_NOTHING:   I've heard reports of some systems defining getopt in
#                     stdlib.h, which unix.c automatically includes.
#                     (*BSD, Solaris?)
#
# If none of the above are defined, frotz will define the getopt prototypes
# itself.
#
#    -DUSE_NCURSES_H: Use this if you want to include ncurses.h rather
#                     than curses.h.
#
# These defines add various cosmetic features to the interpreter:
#    -DCOLOR_SUPPORT: If the terminal you're using has color support, frotz
#                     will use the curses color routines...if your curses
#                     library supports color.
#    -DEMACS_EDITING: This enables some of the standard emacs editing keys
#                     (Ctrl-B,-F,-P,-N, etc.) to be used when entering
#                     input.  I can't see any reason why you wouldn't want
#                     to define it--it can't hurt anything--but you do
#                     have that option.
#
#DEFS = -DUSE_GETOPT_H -DCOLOR_SUPPORT -DEMACS_EDITING
DEFS =

# This should point to the location of your curses or ncurses include file
# if it's in a non-standard place.
#INCL = -I/usr/local/include
#INCL = -I/5usr/include
INCL =

# This should define the location and name of whatever curses library you're
# linking with.
#LIB = -L/usr/local/lib
#CURSES = -lncurses
#LIB = -L/5usr/lib
LIB =
CURSES = -lcurses

# Nothing under this line should need to be changed.

OBJECTS = buffer.o fastmem.o files.o hotkey.o input.o main.o math.o object.o \
          process.o random.o redirect.o screen.o sound.o stream.o table.o \
          text.o ux_init.o ux_input.o ux_pic.o ux_screen.o ux_sample.o \
          ux_text.o variable.o

CFLAGS = $(OPTS) $(DEFS) $(INCL)

frotz: $(OBJECTS)
	$(CC) -o frotz $(OBJECTS) $(LIB) $(CURSES)

