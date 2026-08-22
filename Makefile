#
# Simbox Native SDK - Root Master Makefile
#

CC ?= gcc
AR ?= ar
RANLIB ?= ranlib

CFLAGS ?= -O2 -Wall -fPIC -DHAVE_CONFIG_H
INCLUDES = -I./adapters/include -I./src -I./asterisk_chan_svistok/chan_svistok

# OS-specific shared library extension & linker flags
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    SHLIB_EXT = dylib
    SHLIB_FLAGS = -dynamiclib
    LDFLAGS_EXTRA = -liconv
else
    SHLIB_EXT = so
    SHLIB_FLAGS = -shared
    LDFLAGS_EXTRA =
endif

# Sources
ADAPTER_SRCS = $(wildcard adapters/src/*.c)
ADAPTER_OBJS = $(ADAPTER_SRCS:.c=.o)

SRC_SRCS = $(wildcard src/*.c)
SRC_OBJS = $(SRC_SRCS:.c=.o)

SVISTOK_DIR = asterisk_chan_svistok/chan_svistok
SVISTOK_SRCS = \
	$(SVISTOK_DIR)/char_conv.c \
	$(SVISTOK_DIR)/pdu.c \
	$(SVISTOK_DIR)/mixbuffer.c \
	$(SVISTOK_DIR)/ringbuffer.c \
	$(SVISTOK_DIR)/memmem.c \
	$(SVISTOK_DIR)/helpers.c \
	$(SVISTOK_DIR)/cpvt.c \
	$(SVISTOK_DIR)/at_parse.c \
	$(SVISTOK_DIR)/at_read.c \
	$(SVISTOK_DIR)/dc_config.c \
	$(SVISTOK_DIR)/pdiscovery.c \
	$(SVISTOK_DIR)/at_command.c \
	$(SVISTOK_DIR)/at_queue.c \
	$(SVISTOK_DIR)/at_response.c \
	$(SVISTOK_DIR)/cli.c \
	$(SVISTOK_DIR)/app.c \
	$(SVISTOK_DIR)/manager.c \
	$(SVISTOK_DIR)/dsp.c \
	$(SVISTOK_DIR)/channel.c \
	$(SVISTOK_DIR)/chan_dongle.c

SVISTOK_OBJS = $(SVISTOK_SRCS:.c=.o)

LIB_A = libsimbox.a
LIB_SH = libsimbox.$(SHLIB_EXT)
TEST_BIN = test_simbox

all: $(LIB_A) $(LIB_SH) $(TEST_BIN)

# Object compilation rules
adapters/src/%.o: adapters/src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(SVISTOK_DIR)/%.o: $(SVISTOK_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -Wno-return-mismatch -Wno-return-type -Wno-pointer-sign -c $< -o $@

# Static library
$(LIB_A): $(ADAPTER_OBJS) $(SRC_OBJS) $(SVISTOK_OBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

# Shared library
$(LIB_SH): $(ADAPTER_OBJS) $(SRC_OBJS) $(SVISTOK_OBJS)
	$(CC) $(SHLIB_FLAGS) -o $@ $^ -lpthread -lm $(LDFLAGS_EXTRA)

# Test binary
$(TEST_BIN): tests/test_simbox.c $(LIB_A)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lsimbox -lpthread -lm $(LDFLAGS_EXTRA) -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

test-real-wiring: all
	./tests/test_real_wiring.sh

clean:
	rm -f $(ADAPTER_OBJS) $(SRC_OBJS) $(SVISTOK_OBJS) $(LIB_A) $(LIB_SH) $(TEST_BIN)
	make -C adapters clean

.PHONY: all test test-real-wiring clean
