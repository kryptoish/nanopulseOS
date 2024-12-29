# Variables
CFLAGS ?= -O2 -g
CPPFLAGS ?= -Iinclude
LDFLAGS ?=
LIBS ?=

DESTDIR ?=
PREFIX ?=/usr/local
EXEC_PREFIX ?=$(PREFIX)
INCLUDEDIR ?=$(PREFIX)/include
LIBDIR ?=$(EXEC_PREFIX)/lib

CFLAGS := $(CFLAGS) -ffreestanding -Wall -Wextra
CPPFLAGS := $(CPPFLAGS)

# Source files and object files
SRCS := \
    src/idt.c \
    src/isr.c \
    src/timer.c

OBJS := $(SRCS:.c=.o)

# Output library
LIBDRIVERS := libdrivers.a

# Targets
.PHONY: all clean install install-headers install-libs
.SUFFIXES: .o .c

all: $(LIBDRIVERS)

$(LIBDRIVERS): $(OBJS)
	$(AR) rcs $@ $(OBJS)

.c.o:
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f $(LIBDRIVERS) $(OBJS) */*.o
	rm -f $(OBJS:.o=.d) */*.d

install: install-headers install-libs

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)/drivers
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/drivers

install-libs: $(LIBDRIVERS)
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp $(LIBDRIVERS) $(DESTDIR)$(LIBDIR)

-include $(OBJS:.o=.d)
