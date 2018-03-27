# For GNU Make.

SOURCES = dumb_blorb.c dumb_init.c dumb_input.c dumb_output.c dumb_pic.c

OBJECTS = $(SOURCES:.c=.o)

TARGET = frotz_dumb.a

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
