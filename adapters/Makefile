#
# Makefile for Asterisk Shim Library (libsimbox_shim.a)
#

CC ?= gcc
AR ?= ar
RANLIB ?= ranlib
CFLAGS ?= -O2 -Wall -fPIC -DHAVE_CONFIG_H
INCLUDES = -I./include

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)
LIB = libsimbox_shim.a

all: $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(LIB): $(OBJS)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

clean:
	rm -f $(OBJS) $(LIB)

.PHONY: all clean
