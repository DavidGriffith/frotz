# For GNU Make.

SDL_PKGS ?= libpng libjpeg libsdl SDL_mixer freetype2 zlib

CFLAGS += `pkg-config $(SDL_PKGS) --cflags`
ARFLAGS = rvU

SOURCES = sf_aiffwav.c sf_fonts.c sf_msg_en.c sf_resource.c sf_util.c \
	sf_deffont.c sf_ftype.c sf_osfdlg.c sf_sig.c sf_video.c \
	sf_font3.c sf_images.c sf_sound.c generic.c

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
