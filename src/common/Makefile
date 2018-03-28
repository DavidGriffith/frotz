# For GNU Make.

SOURCES = buffer.c err.c fastmem.c files.c getopt.c hotkey.c input.c \
	main.c math.c object.c process.c quetzal.c random.c redirect.c \
	screen.c sound.c stream.c table.c text.c variable.c version.c

HEADERS = frotz.h setup.h unused.h

OBJECTS = $(SOURCES:.c=.o)

TARGET = frotz_common.a

ARFLAGS = rc

.PHONY: clean
.DELETE_ON_ERROR:

$(TARGET): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $?
	ranlib $@

clean:
	rm -f $(TARGET) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -fpic -o $@ -c $<
