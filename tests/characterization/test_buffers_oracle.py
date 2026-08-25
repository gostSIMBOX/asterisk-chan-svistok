#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path
import sys
import unittest


CHARACTERIZATION_ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(CHARACTERIZATION_ROOT))

from harness import compile_and_run  # noqa: E402


BUFFER_DRIVER = r'''
#include <stdio.h>
#include <string.h>
#include "ringbuffer.h"
#include "mixbuffer.h"

static void print_iov(const char *name, int count, struct iovec iov[2])
{
    int index;
    printf("%s|%d|", name, count);
    for (index = 0; index < count; ++index)
        printf("%.*s", (int)iov[index].iov_len, (char *)iov[index].iov_base);
    printf("|%zu|%zu\n", count > 0 ? iov[0].iov_len : 0,
        count > 1 ? iov[1].iov_len : 0);
}

int main(void)
{
    char storage[8] = {0};
    struct ringbuffer rb;
    struct iovec iov[2];
    int count;
    size_t amount;
    short mix_storage[4] = {0};
    short first_samples[2] = {1000, 30000};
    short second_samples[2] = {2000, 10000};
    struct mixbuffer mb;
    struct mixstream first_stream;
    struct mixstream second_stream;
    size_t first_write;
    size_t second_write;

    rb_init(&rb, storage, sizeof(storage));
    printf("initial|%zu|%zu\n", rb_used(&rb), rb_free(&rb));
    printf("write-a|%zu\n", rb_write(&rb, "abcdef", 6));
    printf("read-upd|%zu\n", rb_read_upd(&rb, 5));
    printf("write-b|%zu\n", rb_write(&rb, "WXYZ", 4));
    count = rb_read_all_iov(&rb, iov);
    print_iov("wrapped", count, iov);
    count = rb_read_until_char_after_iov(&rb, iov, 'X', 1);
    print_iov("after-X", count, iov);
    count = rb_read_until_mem_iov(&rb, iov, "XYZ", 3);
    print_iov("cross-XYZ", count, iov);
    amount = rb_write(&rb, "123456789", 9);
    printf("overflow-write|%zu|%zu|%zu\n", amount, rb_used(&rb), rb_free(&rb));
    amount = rb_read_upd(&rb, 99);
    printf("over-read|%zu|%zu|%zu\n", amount, rb_used(&rb), rb_free(&rb));

    mixb_init(&mb, mix_storage, sizeof(mix_storage));
    mixb_attach(&mb, &first_stream);
    mixb_attach(&mb, &second_stream);
    printf("mix-attach|%d\n", mixb_streams(&mb));
    first_write = mixb_write(&mb, &first_stream,
        (const char *)first_samples, sizeof(first_samples));
    second_write = mixb_write(&mb, &second_stream,
        (const char *)second_samples, sizeof(second_samples));
    printf("mix-write|%zu|%zu\n", first_write, second_write);
    printf("mix-result|%d|%d|%zu\n", mix_storage[0], mix_storage[1], mixb_used(&mb));
    mixb_read_upd(&mb, sizeof(first_samples));
    printf("mix-read|%zu|%zu|%zu\n", mixb_used(&mb), first_stream.used, second_stream.used);
    mixb_detach(&mb, &first_stream);
    mixb_detach(&mb, &second_stream);
    printf("mix-detach|%d\n", mixb_streams(&mb));
    return 0;
}
'''


class BufferOracleTests(unittest.TestCase):
    def test_ringbuffer_boundaries_match_frozen_legacy_output(self) -> None:
        output = compile_and_run(
            "ringbuffer-boundaries",
            BUFFER_DRIVER,
            ("ringbuffer.c", "memmem.c", "mixbuffer.c"),
            compiler_flags=("-include", "string.h"),
        )
        expected = (
            CHARACTERIZATION_ROOT / "fixtures" / "buffers" / "ringbuffer.txt"
        ).read_text(encoding="utf-8")
        self.assertEqual(expected, output)


if __name__ == "__main__":
    unittest.main()
