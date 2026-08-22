# Phase 0 baseline harness — builds chan_svistok's OWN existing test
# files (read-only, unmodified) against the shim, to establish a
# verified behavioral baseline before any adapter/API code is touched.
# See flows/sdd-asterisk-chan-simbox/04-implementation-log.md for full
# per-file results and why only test1 builds standalone here.

CC ?= gcc
SVISTOK = ../../asterisk_chan_svistok/chan_svistok
CFLAGS = -O2 -Wall -fPIC -DHAVE_CONFIG_H -std=gnu89 -Wno-error=implicit-function-declaration
INCLUDES = -I../../adapters/include -I../../src -I$(SVISTOK)
LDFLAGS = -lpthread -lm -liconv
LIBSIMBOX = ../../libsimbox.a

.PHONY: all clean
all: test1

# test1: mixbuffer/ringbuffer logic, no hardware, no I/O — links against
# the already-built libsimbox.a (which already contains mixbuffer.o
# etc.) rather than recompiling chan_svistok sources separately.
test1: $(SVISTOK)/test/test1.c $(LIBSIMBOX)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(LIBSIMBOX) $(LDFLAGS) -o $@

# adiscovery_test, ttyprog_test, reader_test: NOT built here. Each has a
# genuine, pre-existing standalone-build gap unrelated to adapter work
# (undeclared identifiers needing chan_dongle.c's own TU context;
# missing headers; symbols split across files these test files don't
# themselves include) — see the implementation log's Task 0.2 entry for
# the exact errors and what was tried. All three also require real
# ttyUSB hardware to be behaviorally meaningful even once built.

clean:
	rm -f test1
