# For GNU Make.

SOURCES = ux_audio.c ux_audio_none.c ux_audio_oss.c ux_blorb.c ux_init.c \
	ux_input.c ux_locks.c ux_pic.c ux_resource.c ux_screen.c ux_text.c

HEADERS = ux_blorb.h ux_frotz.h ux_locks.h ux_messages.h ux_setup.h defines.h

OBJECTS = $(SOURCES:.c=.o)

TARGET = frotz_curses.a

ARFLAGS = rc

SOUND ?= ao

ifeq ($(SOUND), ao)
  CFLAGS += -pthread
else ifeq ($(SOUND), none)
  CFLAGS += -DNO_SOUND
else ifndef SOUND
  CFLAGS += -DNO_SOUND
else
  $(error Invalid sound choice $(SOUND))
endif

.PHONY: clean
.DELETE_ON_ERROR:

$(TARGET): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $?
	ranlib $@

clean:
	rm -f $(TARGET) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -fpic -o $@ -c $<
