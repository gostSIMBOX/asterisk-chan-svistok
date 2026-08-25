#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path
import sys
import unittest


CHARACTERIZATION_ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(CHARACTERIZATION_ROOT))

from harness import compile_and_run  # noqa: E402


AT_READ_DRIVER = r'''
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "at_read.h"
#include "at_response.h"
#include "ringbuffer.h"

const at_responses_t at_responses = { 0, 0, 0, 0, 0 };

void ast_debug(int level, const char *format, ...)
{
    (void)level;
    (void)format;
}

void ast_log(int level, const char *format, ...)
{
    (void)level;
    (void)format;
}

int ast_waitfor_n_fd(int *fds, int count, int *milliseconds, int *exception)
{
    (void)count;
    *exception = 0;
    if (*milliseconds <= 0)
        return 0;
    *milliseconds = 0;
    return fds[0];
}

static void print_iov_hex(int count, struct iovec iov[2])
{
    int index;
    size_t offset;
    for (index = 0; index < count; ++index)
        for (offset = 0; offset < iov[index].iov_len; ++offset)
            printf("%02X", ((unsigned char *)iov[index].iov_base)[offset]);
}

int main(void)
{
    char storage[64] = {0};
    struct ringbuffer rb;
    struct iovec iov[2];
    int descriptors[2];
    int timeout = 25;
    int read_result = 0;
    int count;
    ssize_t amount;

    rb_init(&rb, storage, sizeof(storage));
    pipe(descriptors);
    write(descriptors[1], "\r\nOK\r\n", 6);
    close(descriptors[1]);
    printf("wait|%d|%d\n", at_wait(descriptors[0], &timeout), timeout);
    amount = at_read(descriptors[0], "fixture", &rb);
    close(descriptors[0]);
    printf("read|%zd|%zu\n", amount, rb_used(&rb));
    count = at_read_result_iov("fixture", &read_result, &rb, iov);
    printf("result|%d|%d|", count, read_result);
    print_iov_hex(count, iov);
    printf("|%zu\n", rb_used(&rb));
    rb_read_upd(&rb, count ? iov[0].iov_len + iov[1].iov_len : 0);
    printf("remaining|%zu\n", rb_used(&rb));
    return 0;
}
'''


class AtReadOracleTests(unittest.TestCase):
    def test_at_read_boundaries_match_frozen_legacy_output(self) -> None:
        output = compile_and_run(
            "at-read",
            AT_READ_DRIVER,
            ("at_read.c", "ringbuffer.c", "memmem.c"),
            compiler_defines=("-DCHAN_DONGLE_H_INCLUDED",),
            compiler_flags=(
                "-include",
                "asterisk_compat.h",
                "-include",
                "string.h",
            ),
        )
        expected = (
            CHARACTERIZATION_ROOT / "fixtures" / "buffers" / "at-read.txt"
        ).read_text(encoding="utf-8")
        self.assertEqual(expected, output)


if __name__ == "__main__":
    unittest.main()
