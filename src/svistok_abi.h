#ifndef SVISTOK_ABI_H_INCLUDED
#define SVISTOK_ABI_H_INCLUDED

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#include "ringbuffer.h"
#include "at_response.h"
#include "dc_config.h"
#include "at_command.h"
#include "chan_dongle.h"
#include <asterisk-chan-dongle/at_queue.h>

#define SVISTOK_ABI_ASSERT(name, expression) \
    typedef char svistok_abi_assert_##name[(expression) ? 1 : -1]

#ifndef SVISTOK_ABI_EXPECT_CALL_STATE_WAITING
#define SVISTOK_ABI_EXPECT_CALL_STATE_WAITING 5
#endif

SVISTOK_ABI_ASSERT(call_state_active, CALL_STATE_ACTIVE == 0);
SVISTOK_ABI_ASSERT(call_state_waiting,
    CALL_STATE_WAITING == SVISTOK_ABI_EXPECT_CALL_STATE_WAITING);
SVISTOK_ABI_ASSERT(call_state_released, CALL_STATE_RELEASED == 6);
SVISTOK_ABI_ASSERT(call_state_init, CALL_STATE_INIT == 7);
SVISTOK_ABI_ASSERT(call_state_count, CALL_STATES_NUMBER == 8);
SVISTOK_ABI_ASSERT(call_flag_master, CALL_FLAG_MASTER == 32);
SVISTOK_ABI_ASSERT(call_flag_multipart, CALL_FLAG_MULTIPARTY == 256);

SVISTOK_ABI_ASSERT(cpvt_entry_first, offsetof(struct cpvt, entry) == 0);
SVISTOK_ABI_ASSERT(cpvt_channel_order,
    offsetof(struct cpvt, channel) < offsetof(struct cpvt, requestor));
SVISTOK_ABI_ASSERT(cpvt_owner_order,
    offsetof(struct cpvt, requestor) < offsetof(struct cpvt, pvt));
SVISTOK_ABI_ASSERT(cpvt_frame_order,
    offsetof(struct cpvt, a_read_buf) < offsetof(struct cpvt, a_read_frame));
SVISTOK_ABI_ASSERT(cpvt_answered_last,
    offsetof(struct cpvt, a_read_frame) < offsetof(struct cpvt, answered));
SVISTOK_ABI_ASSERT(cpvt_audio_buffer_size,
    sizeof(((struct cpvt *)0)->a_read_buf) == FRAME_SIZE + AST_FRIENDLY_OFFSET);

SVISTOK_ABI_ASSERT(pvt_entry_first, offsetof(struct pvt, entry) == 0);
SVISTOK_ABI_ASSERT(pvt_config_state_order,
    offsetof(struct pvt, settings) < offsetof(struct pvt, state));
SVISTOK_ABI_ASSERT(pvt_state_stat_order,
    offsetof(struct pvt, state) < offsetof(struct pvt, stat));
SVISTOK_ABI_ASSERT(pvt_call_state_slots,
    sizeof(((pvt_state_t *)0)->chan_count) == CALL_STATES_NUMBER);

SVISTOK_ABI_ASSERT(config_unique_shared_order,
    offsetof(pvt_config_t, unique) < offsetof(pvt_config_t, shared));
SVISTOK_ABI_ASSERT(config_device_id_size,
    sizeof(((dc_uconfig_t *)0)->id) == DEVNAMELEN);
SVISTOK_ABI_ASSERT(config_device_path_size,
    sizeof(((dc_uconfig_t *)0)->audio_tty) == DEVPATHLEN);
SVISTOK_ABI_ASSERT(config_serial_size,
    sizeof(((dc_uconfig_t *)0)->serial) == SERIAL_SIZE + 2);
SVISTOK_ABI_ASSERT(config_context_size,
    sizeof(((dc_sconfig_t *)0)->context) == AST_MAX_CONTEXT);

SVISTOK_ABI_ASSERT(at_command_first, CMD_USER == 0);
SVISTOK_ABI_ASSERT(at_command_sysinfo_after_csnr, CMD_AT_SYSINFO == CMD_AT_CSNR + 1);

SVISTOK_ABI_ASSERT(cpvt_alloc_signature,
    __builtin_types_compatible_p(
        __typeof__(&cpvt_alloc),
        struct cpvt *(*)(struct pvt *, int, unsigned, call_state_t)));
SVISTOK_ABI_ASSERT(config_fill_signature,
    __builtin_types_compatible_p(
        __typeof__(&dc_config_fill),
        int (*)(struct ast_config *, const char *, const struct dc_sconfig *,
            struct pvt_config *)));

#endif
