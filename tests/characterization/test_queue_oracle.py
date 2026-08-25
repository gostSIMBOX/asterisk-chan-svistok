#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path
import sys
import unittest


CHARACTERIZATION_ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(CHARACTERIZATION_ROOT))

from harness import compile_and_run  # noqa: E402


QUEUE_DRIVER = r'''
#include "queue_compat.h"

static at_queue_cmd_t command(int cmd, int res, int flags)
{
    at_queue_cmd_t value;
    memset(&value, 0, sizeof(value));
    value.cmd = cmd;
    value.res = res;
    value.flags = flags | ATQ_CMD_FLAG_STATIC;
    return value;
}

static void print_queue(const char *label, struct pvt *pvt)
{
    const at_queue_task_t *task = at_queue_head_task(pvt);
    printf("%s|state=%d,%d|stat=%d,%d|head=%d,%u/%u\n",
        label,
        pvt->state.at_tasks, pvt->state.at_cmds,
        pvt->stat.at_tasks, pvt->stat.at_cmds,
        task ? task->cmds[task->cindex].cmd : -1,
        task ? task->cindex : 0,
        task ? task->cmdsno : 0);
}

int main(void)
{
    struct pvt pvt;
    struct cpvt cpvt;
    at_queue_cmd_t first[2];
    at_queue_cmd_t priority[2];
    at_queue_cmd_t tail[1];
    int first_result;
    int priority_result;
    int tail_result;

    memset(&pvt, 0, sizeof(pvt));
    pvt.at_queue.last = &pvt.at_queue.first;
    pvt.id = "oracle";
    cpvt.pvt = &pvt;

    first[0] = command(10, 100, 0);
    first[1] = command(11, 101, 0);
    priority[0] = command(20, 200, ATQ_CMD_FLAG_IGNORE);
    priority[1] = command(21, 201, 0);
    tail[0] = command(30, 300, 0);

    first_result = at_queue_insert_const(&cpvt, first, 2, 0);
    priority_result = at_queue_insert_const(&cpvt, priority, 2, 1);
    tail_result = at_queue_insert_const(&cpvt, tail, 1, 0);
    printf("insert|%d|%d|%d\n", first_result, priority_result, tail_result);
    print_queue("initial", &pvt);

    at_queue_handle_result(&pvt, 100);
    print_queue("advance", &pvt);
    at_queue_handle_result(&pvt, 999);
    print_queue("mismatch-removes-task", &pvt);
    at_queue_handle_result(&pvt, 999);
    print_queue("ignore-mismatch", &pvt);
    at_queue_flush(&pvt);
    print_queue("flush", &pvt);
    return 0;
}
'''


class QueueOracleTests(unittest.TestCase):
    def test_queue_transitions_match_frozen_legacy_output(self) -> None:
        output = compile_and_run(
            "at-queue",
            QUEUE_DRIVER,
            ("at_queue.c",),
            compiler_flags=(
                "-include",
                str(CHARACTERIZATION_ROOT / "stubs" / "queue_compat.h"),
            ),
        )
        expected = (
            CHARACTERIZATION_ROOT / "fixtures" / "state" / "queue.txt"
        ).read_text(encoding="utf-8")
        self.assertEqual(expected, output)


if __name__ == "__main__":
    unittest.main()
