# Define your C compiler.  I recommend gcc if you have it.
# MacOS users should use "cc" even though it's really "gcc".
#
CC = gcc
#CC = cc

# Define your optimization flags.  Most compilers understand -O and -O2,
# Standard (note: Solaris on UltraSparc using gcc 2.8.x might not like this.)
#
OPTS = -O2
# Pentium with gcc 2.7.0 or better
#OPTS = -O2 -fomit-frame-pointer -malign-functions=2 -malign-loops=2 \
#-malign-jumps=2

# Define where you want Frotz to be installed.  Usually this is /usr/local
#
PREFIX = /usr/local
#PREFIX =

#Define where manpages should go.
#
MAN_PREFIX = $(PREFIX)
#MAN_PREFIX = /usr/local/share/

# Define where you want Frotz to look for frotz.conf.
#
CONFIG_DIR = /usr/local/etc
#CONFIG_DIR = /etc
#CONFIG_DIR = /usr/pkg/etc
#CONFIG_DIR =

# Uncomment this if you want color support.  Usually this requires ncurses.
#
COLOR_DEFS = -DCOLOR_SUPPORT

# Uncomment this if you have an OSS soundcard driver and want classical
# Infocom sound support.  Currently this works only for Linux.
#
#SOUND_DEFS = -DOSS_SOUND

# Also uncomment this if you want sound through the OSS driver.
#
#SOUND_LIB = -lossaudio

# This should point to the location of your curses.h or ncurses.h include
# file if your compiler doesn't know about it.
#
INCL = -I/usr/local/include
#INCL = -I/usr/pkg/include
#INCL = -I/usr/freeware/include
#INCL = -I/5usr/include
#INCL =

# This should define the location and name of whatever curses library you're
# linking with.  Usually, this isn't necessary if /etc/ld.so.conf is set
# up correctly.
#
LIB = -L/usr/local/lib
#LIB = -L/usr/pkg/lib
#LIB = -L/usr/freeware/lib
#LIB = -L/5usr/lib
#LIB =

# One of these must be uncommented, use ncurses if you have it.
#
CURSES = -lncurses	# Linux always uses ncurses.
#CURSES = -lcurses

# Comment this out if you're using plain old curses.
#
CURSES_DEF = -DUSE_NCURSES_H

# Uncomment this if you're compiling Unix Frotz on a machine that lacks
# the memmove(3) system call.  If you don't know what this means, leave it
# alone.
#
#MEMMOVE_DEF = -DNO_MEMMOVE

# Uncomment this if for some wacky reason you want to compile Unix Frotz
# under Cygwin under Windoze.  This sort of thing is not reccomended.
#
#EXTENSION = .exe


#####################################################
# Nothing under this line should need to be changed.
#####################################################

VERSION = 2.42

BINNAME = frotz

DISTFILES = bugtest

DISTNAME = $(BINNAME)-$(VERSION)
distdir = $(DISTNAME)


OBJECTS = buffer.o err.o fastmem.o files.o hotkey.o input.o main.o \
	math.o object.o process.o quetzal.o random.o redirect.o \
	screen.o sound.o stream.o table.o text.o ux_init.o \
	ux_input.o ux_pic.o ux_screen.o ux_text.o variable.o \
	ux_audio_none.o ux_audio_oss.o

OPT_DEFS = -DCONFIG_DIR="\"$(CONFIG_DIR)\"" $(CURSES_DEF) \
	-DVERSION="\"$(VERSION)\""


COMP_DEFS = $(OPT_DEFS) $(COLOR_DEFS) $(SOUND_DEFS) $(SOUNDCARD) \
	$(MEMMOVE_DEF)

CFLAGS = $(OPTS) $(COMP_DEFS) $(INCL)


$(BINNAME): soundcard.h $(OBJECTS)
	$(CC) -o $(BINNAME)$(EXTENSION) $(OBJECTS) $(LIB) $(CURSES) $(SOUND_LIB)

all: $(BINNAME)

soundcard.h:
	@if [ ! -f soundcard.h ] ; then ./findsound.sh; fi

install: $(BINNAME)
	install -d $(PREFIX)/bin
	install -d $(MAN_PREFIX)/man/man6
	strip $(BINNAME)$(EXTENSION)
	install -c -m 755 $(BINNAME)$(EXTENSION) $(PREFIX)/bin
	install -c -m 644 $(BINNAME).6 $(MAN_PREFIX)/man/man6

uninstall:
	rm -f $(PREFIX)/bin/$(BINNAME)$(EXTENSION)
	rm -f $(MAN_PREFIX)/man/man6/$(BINNAME).6

deinstall: uninstall

clean:
	rm -f *.o

distro: dist

dist: distclean

	mkdir $(distdir)

	@for file in `ls`; do \
		if test $$file != $(distdir); then \
			cp -rp $$file $(distdir)/$$file; \
		fi; \
	done

	tar chof $(distdir).tar $(distdir)
	gzip -f --best $(distdir).tar
	rm -rf $(distdir)

	@echo
	@echo "$(distdir).tar.gz created"
	@echo

distclean: clean
	rm -f soundcard.h
	rm -f $(BINNAME)$(EXTENSION)
	-rm -rf $(distdir)
	-rm -f $(distdir).tar $(distdir).tar.gz

realclean: distclean

clobber: distclean

help:
	@echo
	@echo "Targets:"
	@echo "    frotz"
	@echo "    install"
	@echo "    uninstall"
	@echo "    clean"
	@echo "    distclean"
	@echo
