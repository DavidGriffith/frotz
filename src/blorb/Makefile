# For GNU Make.

SOURCES = blorblib.c

HEADERS = blorb.h blorblow.h

OBJECTS = $(SOURCES:.c=.o)

DEPS = $(SOURCES:.c=.d)

TARGET = blorblib.a

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
