# Define your C compiler.  I recommend gcc if you have it.
# MacOS users should use "cc" even though it's really "gcc".
#
CC = gcc

# Define your optimization flags.
#
# These are good for regular use.
#OPTS = -O2 -fomit-frame-pointer -falign-functions=2 -falign-loops=2 -falign-jumps=2
# These are handy for debugging.
#OPTS = -g -Wall
OPTS = -g

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


#########################################################################
# This section is where Frotz is actually built.
# Under normal circumstances, nothing in this section should be changed.
#########################################################################

SRCDIR = src
VERSION = 2.44
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
		$(CURSES_DIR)/ux_audio_none.o

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

SOUND_LIB = -lao -ldl -lm -lsndfile -lvorbisfile -lmodplug -lsamplerate

#########################################################################
#########################################################################
# Targets
#

$(NAME): $(NAME)-curses
curses:  $(NAME)-curses
$(NAME)-curses: defines $(COMMON_TARGET) $(CURSES_TARGET) $(BLORB_TARGET)

ifeq ($(SOUND), ao)
	$(CC) -o $(BINNAME)$(EXTENSION) $(TARGETS) $(LIB) $(CURSES) $(SOUND_LIB)
else ifeq ($(SOUND), none)
	$(CC) -o $(BINNAME)$(EXTENSION) $(TARGETS) $(LIB) $(CURSES)
else ifndef SOUND
	$(CC) -o $(BINNAME)$(EXTENSION) $(TARGETS) $(LIB) $(CURSES)
else
	@echo "Invalid sound choice $(SOUND)."
endif


dumb:		$(NAME)-dumb
d$(NAME):	$(NAME)-dumb
$(NAME)-dumb:		$(COMMON_TARGET) $(DUMB_TARGET)
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
defines: defines.h
defines.h:
	@echo "Sound support: $(SOUND)"
	@echo "#define VERSION \"$(VERSION)\"" > $(CURSES_DIR)/defines.h
	@echo "#define CONFIG_DIR \"$(CONFIG_DIR)\"" >> $(CURSES_DIR)/defines.h
	@echo "#define SOUND \"$(SOUND)\"" >> src/curses/defines.h

ifeq ($(SOUND), none)
	@echo "#define NO_SOUND" >> $(CURSES_DIR)/defines.h
endif

ifndef SOUND
	@echo "#define NO_SOUND" >> $(CURSES_DIR)/defines.h
endif

ifdef COLOR
	@echo "#define COLOR_SUPPORT" >> $(CURSES_DIR)/defines.h
endif

ifdef NO_MEMMOVE
	@echo "#define NO_MEMMOVE" >> $(CURSES_DIR)/defines.h
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

deinstall: uninstall

install_dumb: d$(NAME)
	strip d$(BINNAME)$(EXTENSION)
	install -d $(PREFIX)/bin
	install -d $(MAN_PREFIX)/man/man6
	install -c -m 755 d$(BINNAME)$(EXTENSION) $(PREFIX)/bin
	install -c -m 644 doc/d$(NAME).6 $(MAN_PREFIX)/man/man6

uninstall_dumb:
	rm -f $(PREFIX)/bin/d$(NAME)
	rm -f $(MAN_PREFIX)/man/man6/d$(NAME).6

deinstall_dumb: uninstall_dumb

distro: dist

dist: distclean
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
	rm -f $(CURSES_DIR)/defines.h
	find . -name *.o -exec rm -f {} \;
	find . -name *.O -exec rm -f {} \;

distclean: clean
	rm -f $(BINNAME)$(EXTENSION) d$(BINNAME)$(EXTENSION) s$(BINNAME)
	rm -f *.EXE *.BAK *.LIB
	rm -f *.exe *.bak *.lib
	rm -f *core $(SRCDIR)/*core
	-rm -rf $(distdir)
	-rm -f $(distdir).tar $(distdir).tar.gz

realclean: distclean
clobber: distclean

help:
	@echo
	@echo "Targets:"
	@echo "    frotz"
	@echo "    dfrotz"
	@echo "    install"
	@echo "    uninstall"
	@echo "    clean"
	@echo "    distclean"
	@echo

