# For GNU Make.

PKGS = sdl SDL_mixer freetype2

CC = gcc
CFLAGS += `pkg-config $(PKGS) --cflags`
ARFLAGS = rvU

SOURCES = sf_aiffwav.c sf_fonts.c sf_msg_en.c sf_resource.c sf_util.c \
	sf_deffont.c sf_ftype.c sf_osfdlg.c sf_sig.c sf_video.c \
	sf_font3.c sf_images.c sf_resample.c sf_sound.c

HEADERS = samplerate.h  sf_frotz.h

OBJECTS = $(SOURCES:.c=.o)

DEPS = $(SOURCES:.c=.d)

TARGET = frotz_sdl.a

.PHONY: clean distclean
.DELETE_ON_ERROR:

$(TARGET): $(TARGET)($(OBJECTS))
	ranlib $@

clean:
	-rm -f $(TARGET) $(OBJECTS)

distclean: clean
	-rm -f $(DEPS)

%.d: %.c
	gcc -MM $(CFLAGS) $< > $@

include $(DEPS)
