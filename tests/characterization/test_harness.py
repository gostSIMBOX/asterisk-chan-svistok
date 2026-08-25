#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path
import sys
import unittest


CHARACTERIZATION_ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(CHARACTERIZATION_ROOT))

from harness import compile_and_run  # noqa: E402


RINGBUFFER_SMOKE_DRIVER = r'''
#include <stdio.h>
#include <string.h>
#include "ringbuffer.h"

int main(void)
{
    char storage[8] = {0};
    struct ringbuffer rb;
    struct iovec iov[2];
    int count;

    rb_init(&rb, storage, sizeof(storage));
    rb_write(&rb, "abc\r", 4);
    count = rb_read_until_char_iov(&rb, iov, '\r');
    printf("{\"count\":%d,\"first_len\":%zu,\"used\":%zu}\n",
        count, count ? iov[0].iov_len : 0, rb_used(&rb));
    return 0;
}
'''


class HarnessTests(unittest.TestCase):
    def test_ringbuffer_oracle_runs_out_of_tree(self) -> None:
        output = compile_and_run(
            "ringbuffer-smoke",
            RINGBUFFER_SMOKE_DRIVER,
            ("ringbuffer.c", "memmem.c"),
        )
        self.assertEqual('{"count":1,"first_len":3,"used":4}\n', output)


if __name__ == "__main__":
    unittest.main()
