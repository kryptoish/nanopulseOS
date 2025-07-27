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
    src/timer.c \
    src/interrupt.s

OBJS := $(SRCS:.c=.o)
OBJS := $(OBJS:.s=.o)

# Output library
LIBINTERRUPTS := libinterrupts.a

# Targets
.PHONY: all clean install install-headers install-libs
.SUFFIXES: .o .c

all: $(LIBINTERRUPTS)

$(LIBINTERRUPTS): $(OBJS)
	$(AR) rcs $@ $(OBJS)

.c.o:
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

.s.o:
	nasm -f elf32 $< -o $@

clean:
	rm -f $(LIBINTERRUPTS) $(OBJS) */*.o
	rm -f $(OBJS:.o=.d) */*.d

install: install-headers install-libs

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)/interrupts
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/interrupts

install-libs: $(LIBINTERRUPTS)
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp $(LIBINTERRUPTS) $(DESTDIR)$(LIBDIR)

-include $(OBJS:.o=.d)
