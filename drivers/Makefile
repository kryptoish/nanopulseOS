# Variables
CFLAGS ?= -O2 -g
CC ?= gcc
CPPFLAGS ?= -Iinclude
LDFLAGS ?=
LIBS ?=
RANLIB ?= $(HOST)-ranlib

DESTDIR ?=
PREFIX ?=/usr/local
EXEC_PREFIX ?=$(PREFIX)
INCLUDEDIR ?=$(PREFIX)/include
LIBDIR ?=$(EXEC_PREFIX)/lib

CFLAGS := $(CFLAGS) -ffreestanding -Wall -Wextra
CPPFLAGS := $(CPPFLAGS) --sysroot=$(SYSROOT)

# Source files and object files
SRCS := \
    src/keyboard.c \
    src/ports.c \
    src/screen.c

OBJS := $(SRCS:.c=.o)

# Output library
LIBDRIVERS := libdrivers.a

# Targets
.PHONY: all clean install install-headers install-libs
.SUFFIXES: .o .c

all: $(LIBDRIVERS)

$(LIBDRIVERS): $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

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
