# For GNU Make.

SOURCES = dumb_blorb.c dumb_init.c dumb_input.c dumb_output.c dumb_pic.c

OBJECTS = $(SOURCES:.c=.o)

DEPS = $(SOURCES:.c=.d)

TARGET = frotz_dumb.a

ARFLAGS = rc

.PHONY: clean
#.DELETE_ON_ERROR:

$(TARGET): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $?
	ranlib $@

clean:
	-rm -f $(TARGET) $(OBJECTS) $(DEPS)

%.d: %.c
	$(CC) -MM $(CFLAGS) $< > $@

include $(DEPS)
