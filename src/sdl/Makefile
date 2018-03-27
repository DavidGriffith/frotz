# For GNU Make.

SDL_PKGS ?= libpng libjpeg sdl SDL_mixer freetype2 zlib

# Dependency generation requires GCC.
CC = gcc
CFLAGS += `pkg-config $(SDL_PKGS) --cflags`

SOURCES = sf_fonts.c sf_msg_en.c sf_resource.c sf_util.c \
	sf_deffont.c sf_ftype.c sf_osfdlg.c sf_sig.c sf_video.c \
	sf_font3.c sf_images.c sf_sound.c generic.c

HEADERS = samplerate.h  sf_frotz.h

OBJECTS = $(SOURCES:.c=.o)

DEPS = $(SOURCES:.c=.d)

TARGET = frotz_sdl.a

.PHONY: clean
.DELETE_ON_ERROR:

$(TARGET): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $?
	ranlib $@

clean:
	-rm -f $(TARGET) $(OBJECTS) $(DEPS)

%.o: %.c
%.o: %.c %.d
	$(CC) -MMD -MF $*.d $(CFLAGS) -c $<

$(DEPS):;

include $(wildcard $(DEPS))
