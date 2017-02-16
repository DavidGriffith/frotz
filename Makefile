# Define your C compiler.  I recommend gcc if you have it.
# MacOS users should use "cc" even though it's really "gcc".
#
CC = gcc

# Enable compiler warnings. This is an absolute minimum.
CFLAGS += -Wall -Wextra

# Define your optimization flags.
#
# These are good for regular use.
#OPTS = -O2 -fomit-frame-pointer -falign-functions=2 -falign-loops=2 -falign-jumps=2
# These are handy for debugging.
OPTS = $(CFLAGS) -g

# Define where you want Frotz installed (typically /usr/local).
#
PREFIX = /usr/local
MAN_PREFIX = $(PREFIX)
CONFIG_DIR = /etc
#CONFIG_DIR = $(PREFIX)/etc

# Pick your sound support.  The most featureful form of sound support is
# through libao.  Comment all of these out if you don't want sound.
#
#SOUND = none
SOUND = ao
#SOUND = sun
#SOUND = oss


##########################################################################
# The configuration options below are intended mainly for older flavors
# of Unix.  For Linux, BSD, and Solaris released since 2003, you can
# ignore this section.
##########################################################################

# If your machine's version of curses doesn't support color...
#
COLOR = yes

# If this matters, you can choose libcurses or libncurses.
#
CURSES = -lncurses
#CURSES = -lcurses

# Just in case your operating system keeps its user-added header files
# somewhere unusual...
#
#INCL = -I/usr/local/include
#INCL = -I/usr/pkg/include
#INCL = -I/usr/freeware/include
#INCL = -I/5usr/include

# Just in case your operating system keeps its user-added libraries
# somewhere unusual...
#
#LIB = -L/usr/local/lib
#LIB = -L/usr/pkg/lib
#LIB = -L/usr/freeware/lib
#LIB = -L/5usr/lib

# Uncomment this if you're compiling Unix Frotz on a machine that lacks
# the memmove(3) system call.  If you don't know what this means, leave it
# alone.
#
#NO_MEMMOVE = yes

# Default sample rate for sound effects.
# All modern sound interfaces can be expected to support 44100 Hz sample
# rates.  Earlier ones, particularly ones in Sun 4c workstations support
# only up to 8000 Hz.
SAMPLERATE = 44100

# Audio buffer size in frames
BUFFSIZE = 512

# Default sample rate converter type
DEFAULT_CONVERTER = SRC_SINC_MEDIUM_QUALITY

#########################################################################
# This section is where Frotz is actually built.
# Under normal circumstances, nothing in this section should be changed.
#########################################################################

SRCDIR = src
VERSION = 2.45pre
NAME = frotz
BINNAME = $(NAME)
DISTFILES = bugtest
DISTNAME = $(BINNAME)-$(VERSION)
distdir = $(DISTNAME)

COMMON_DIR = $(SRCDIR)/common
COMMON_TARGET = $(SRCDIR)/frotz_common.a
COMMON_OBJECT = $(COMMON_DIR)/buffer.o \
		$(COMMON_DIR)/err.o \
		$(COMMON_DIR)/fastmem.o \
		$(COMMON_DIR)/files.o \
		$(COMMON_DIR)/hotkey.o \
		$(COMMON_DIR)/input.o \
		$(COMMON_DIR)/main.o \
		$(COMMON_DIR)/math.o \
		$(COMMON_DIR)/object.o \
		$(COMMON_DIR)/process.o \
		$(COMMON_DIR)/quetzal.o \
		$(COMMON_DIR)/random.o \
		$(COMMON_DIR)/redirect.o \
		$(COMMON_DIR)/screen.o \
		$(COMMON_DIR)/sound.o \
		$(COMMON_DIR)/stream.o \
		$(COMMON_DIR)/table.o \
		$(COMMON_DIR)/text.o \
		$(COMMON_DIR)/variable.o

CURSES_DIR = $(SRCDIR)/curses
CURSES_TARGET = $(SRCDIR)/frotz_curses.a
CURSES_OBJECT = $(CURSES_DIR)/ux_init.o \
		$(CURSES_DIR)/ux_input.o \
		$(CURSES_DIR)/ux_pic.o \
		$(CURSES_DIR)/ux_screen.o \
		$(CURSES_DIR)/ux_text.o \
		$(CURSES_DIR)/ux_blorb.o \
		$(CURSES_DIR)/ux_audio.o \
		$(CURSES_DIR)/ux_resource.o \
		$(CURSES_DIR)/ux_audio_none.o \
		$(CURSES_DIR)/ux_locks.o

DUMB_DIR = $(SRCDIR)/dumb
DUMB_TARGET = $(SRCDIR)/frotz_dumb.a
DUMB_OBJECT =	$(DUMB_DIR)/dumb_init.o \
		$(DUMB_DIR)/dumb_input.o \
		$(DUMB_DIR)/dumb_output.o \
		$(DUMB_DIR)/dumb_pic.o

BLORB_DIR = $(SRCDIR)/blorb
BLORB_TARGET =  $(SRCDIR)/blorblib.a
BLORB_OBJECT =  $(BLORB_DIR)/blorblib.o

TARGETS = $(COMMON_TARGET) $(CURSES_TARGET) $(BLORB_TARGET)

FLAGS = $(OPTS) $(INCL)

SOUND_LIB = -lao -ldl -lpthread -lm -lsndfile -lvorbisfile -lmodplug -lsamplerate

#########################################################################
#########################################################################
# Targets
#

.PHONY: all help dist clean distclean install install_dumb uninstall uninstall_dumb hash

$(NAME): hash $(COMMON_DIR)/defines.h $(CURSES_DIR)/curses_defines.h $(COMMON_TARGET) $(CURSES_TARGET) $(BLORB_TARGET)
ifeq ($(SOUND), ao)
	$(CC) -o $(BINNAME)$(EXTENSION) $(TARGETS) $(LIB) $(CURSES) $(SOUND_LIB)
else ifeq ($(SOUND), none)
	$(CC) -o $(BINNAME)$(EXTENSION) $(TARGETS) $(LIB) $(CURSES)
else ifndef SOUND
	$(CC) -o $(BINNAME)$(EXTENSION) $(TARGETS) $(LIB) $(CURSES)
else
	@echo "Invalid sound choice $(SOUND)."
endif


d$(NAME):		$(COMMON_TARGET) $(DUMB_TARGET)
	$(CC) -o d$(BINNAME)$(EXTENSION) $(COMMON_TARGET) $(DUMB_TARGET) $(LIB)

all:	$(NAME) d$(NAME)


.SUFFIXES:
.SUFFIXES: .c .o .h

$(COMMON_OBJECT): %.o: %.c
	$(CC) $(OPTS) -o $@ -c $<

$(BLORB_OBJECT): %.o: %.c
	$(CC) $(OPTS) -o $@ -c $<

$(DUMB_OBJECT): %.o: %.c
	$(CC) $(OPTS) -o $@ -c $<

$(CURSES_OBJECT): %.o: %.c
	$(CC) $(OPTS) -o $@ -c $<


####################################
# Get the defines set up just right
#
$(COMMON_DIR)/defines.h:
	@echo "Generating $@"
	@echo "#define VERSION \"$(VERSION)\"" > $@

$(CURSES_DIR)/curses_defines.h:
	@echo "Generating $@"
	@echo "#define CONFIG_DIR \"$(CONFIG_DIR)\"" >> $@
	@echo "#define SOUND \"$(SOUND)\"" >> $@
	@echo "#define SAMPLERATE $(SAMPLERATE)" >> $@
	@echo "#define BUFFSIZE $(BUFFSIZE)" >> $@
	@echo "#define DEFAULT_CONVERTER $(DEFAULT_CONVERTER)" >> $@

ifeq ($(SOUND), none)
	@echo "#define NO_SOUND" >> $@
endif

ifndef SOUND
	@echo "#define NO_SOUND" >> $@
endif

ifdef COLOR
	@echo "#define COLOR_SUPPORT" >> $@
endif

ifdef NO_MEMMOVE
	@echo "#define NO_MEMMOVE" >> $@
endif


########################################################################
# If you're going to make this target manually, you'd better know which
# config target to make first.
#
common_lib:	$(COMMON_TARGET)
$(COMMON_TARGET): $(COMMON_OBJECT)
	@echo
	@echo "Archiving common code..."
	ar rc $(COMMON_TARGET) $(COMMON_OBJECT)
	ranlib $(COMMON_TARGET)
	@echo

curses_lib:	$(CURSES_TARGET)
$(CURSES_TARGET): $(CURSES_OBJECT)
	@echo
	@echo "Archiving curses interface code..."
	ar rc $(CURSES_TARGET) $(CURSES_OBJECT)
	ranlib $(CURSES_TARGET)
	@echo

dumb_lib:	$(DUMB_TARGET)
$(DUMB_TARGET): $(DUMB_OBJECT)
	@echo
	@echo "Archiving dumb interface code..."
	ar rc $(DUMB_TARGET) $(DUMB_OBJECT)
	ranlib $(DUMB_TARGET)
	@echo

blorb_lib:	$(BLORB_TARGET)
$(BLORB_TARGET): $(BLORB_OBJECT)
	@echo
	@echo "Archiving Blorb file handling code..."
	ar rc $(BLORB_TARGET) $(BLORB_OBJECT)
	ranlib $(BLORB_TARGET)
	@echo

install: $(NAME)
	strip $(BINNAME)$(EXTENSION)
	install -d $(PREFIX)/bin
	install -d $(MAN_PREFIX)/man/man6
	install -c -m 755 $(BINNAME)$(EXTENSION) $(PREFIX)/bin
	install -c -m 644 doc/$(NAME).6 $(MAN_PREFIX)/man/man6

uninstall:
	rm -f $(PREFIX)/bin/$(NAME)
	rm -f $(MAN_PREFIX)/man/man6/$(NAME).6

install_dumb: d$(NAME)
	strip d$(BINNAME)$(EXTENSION)
	install -d $(PREFIX)/bin
	install -d $(MAN_PREFIX)/man/man6
	install -c -m 755 d$(BINNAME)$(EXTENSION) $(PREFIX)/bin
	install -c -m 644 doc/d$(NAME).6 $(MAN_PREFIX)/man/man6

uninstall_dumb:
	rm -f $(PREFIX)/bin/d$(NAME)
	rm -f $(MAN_PREFIX)/man/man6/d$(NAME).6

dist: distclean hash
	mkdir $(distdir)
	@for file in `ls`; do \
		if test $$file != $(distdir); then \
			cp -Rp $$file $(distdir)/$$file; \
		fi; \
	done
	find $(distdir) -type l -exec rm -f {} \;
	tar chof $(distdir).tar $(distdir)
	gzip -f --best $(distdir).tar
	rm -rf $(distdir)
	@echo
	@echo "$(distdir).tar.gz created"
	@echo

clean:
	rm -f $(SRCDIR)/*.h $(SRCDIR)/*.a
	rm -f $(COMMON_DIR)/defines.h
	rm -f $(COMMON_DIR)/git_hash.h
	rm -f $(CURSES_DIR)/curses_defines.h
	find . -name *.o -exec rm -f {} \;
	find . -name *.O -exec rm -f {} \;

distclean: clean
	rm -f $(BINNAME)$(EXTENSION) d$(BINNAME)$(EXTENSION) s$(BINNAME)
	rm -f *.EXE *.BAK *.LIB
	rm -f *.exe *.bak *.lib
	rm -f *core $(SRCDIR)/*core
	-rm -rf $(distdir)
	-rm -f $(distdir).tar $(distdir).tar.gz

# If we're building from a Git repository, fetch the commit tag and put 
#   it into $(COMMON_DIR)/git_hash.h.
# If not, that should mean that we're building from a tarball.  In that 
#  case, $(COMMON_DIR)/git_hash.h should already be there.
hash:
ifneq ($(and $(wildcard .git),$(shell which git)),)
	@echo "Creating $(COMMON_DIR)/git_hash.h"
	@echo "#define GIT_HASH \"$$(git rev-parse HEAD)\"" > $(COMMON_DIR)/git_hash.h
	@echo "#define GIT_TAG \"$$(git describe --tags)\"" >> $(COMMON_DIR)/git_hash.h
	@echo "#define GIT_BRANCH \"$$(git rev-parse --abbrev-ref HEAD)\"" >> $(COMMON_DIR)/git_hash.h
else
  ifneq ($(wildcard $(COMMON_DIR)/git_hash.h),)
	@echo "Found $(COMMON_DIR)/git_hash.h"
  else
	$(error $(COMMON_DIR)/git_hash.h is missing!.)
  endif
endif

help:
	@echo
	@echo "Targets:"
	@echo "    frotz"
	@echo "    dfrotz"
	@echo "    install"
	@echo "    uninstall"
	@echo "    install_dfrotz"
	@echo "    uninstall_dfrotz"
	@echo "    clean"
	@echo "    distclean"
	@echo "    dist"
	@echo
