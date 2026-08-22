CC ?= cc
ROOT := ..
SVISTOK := $(ROOT)/asterisk_chan_svistok/chan_svistok
CFLAGS := -O2 -Wall -fPIC -DHAVE_CONFIG_H -std=gnu89
INCLUDES := -I$(ROOT)/adapters/include -I$(ROOT)/src -I$(SVISTOK)
LDFLAGS := -lpthread -lm
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
LDFLAGS += -liconv
endif

.PHONY: all verify-copies run-copied-test1 clean

all: verify-copies copied-test1

verify-copies:
	cmp $(SVISTOK)/test/test1.c copied/test1.c
	cmp $(SVISTOK)/simnode/adiscovery_test.c copied/adiscovery_test.c
	cmp $(SVISTOK)/programmator/ttyprog_test.c copied/ttyprog_test.c
	cmp $(SVISTOK)/reader/old/test.c copied/reader_test.c

copied-test1: copied/test1.c $(ROOT)/libsimbox.a
	$(CC) $(CFLAGS) $(INCLUDES) $< $(ROOT)/libsimbox.a $(LDFLAGS) -o $@

run-copied-test1: all
	./copied-test1

clean:
	rm -f copied-test1
