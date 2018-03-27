# For GNU Make.

SOURCES = blorblib.c

HEADERS = blorb.h blorblow.h

OBJECTS = $(SOURCES:.c=.o)

TARGET = blorblib.a

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
