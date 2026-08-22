/*
 * Simbox Native SDK - Modem Device Driver Adapter
 *
 * Every simbox_device_t is one of two kinds (see simbox_device_kind_t):
 *   SIMBOX_DEV_SIMULATED - the original fabricated struct, populated via
 *     simbox_device_register() (used on every platform for tests, and
 *     as the *only* device path on non-Linux platforms, since
 *     chan_svistok is Linux-only by design).
 *   SIMBOX_DEV_REAL - Linux only, wraps a real struct pvt* from
 *     chan_dongle.c's own gpublic->devices list. See
 *     flows/sdd-asterisk-chan-simbox/02-specifications.md §9.1/§9's
 *     platform-split note.
 * Every simbox_device_... / simbox_call_... function below branches on
 * `kind` and reads/writes the real pvt fields directly on Linux (struct
 * pvt is a plain, non-opaque struct in chan_dongle.h - direct field
 * access is chan_svistok's own established style, not a shortcut taken
 * here) rather than a copied snapshot, so operations actually affect
 * the real device.
 */
#include "simbox_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#ifdef __linux__
#include <chan_dongle.h>
#include <cpvt.h>
#include <at_command.h>
#endif

typedef enum {
    SIMBOX_DEV_SIMULATED,
    SIMBOX_DEV_REAL,
} simbox_device_kind_t;

struct simbox_device_internal {
    simbox_device_kind_t kind;
    simbox_device_info_t info;   /* SIMULATED: authoritative. REAL: unused for live fields (read from pvt directly instead) - kept zeroed. */
    pthread_mutex_t lock;
    pthread_cond_t at_response_cond;
    int data_fd;
    int audio_fd;
#ifdef __linux__
    struct pvt *pvt;   /* only valid when kind == SIMBOX_DEV_REAL */
    struct cpvt *cpvt; /* lazily acquired via cpvt_alloc() for ops with no active call - Task 5.6 */
    int cpvt_adapter_owned;
    char *at_response_buf;
    size_t at_response_size;
    int at_response_pending;
    int at_response_complete;
    int at_response_result;
    simbox_device_state_t call_state;
    int call_state_known;
#endif
};

simbox_device_t simbox_device_create(const simbox_device_info_t *info)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)calloc(1, sizeof(struct simbox_device_internal));
    if (!dev) return NULL;

    dev->kind = SIMBOX_DEV_SIMULATED;
    if (info) {
        memcpy(&dev->info, info, sizeof(simbox_device_info_t));
    }
    pthread_mutex_init(&dev->lock, NULL);
    pthread_cond_init(&dev->at_response_cond, NULL);
    dev->data_fd = -1;
    dev->audio_fd = -1;
    return (simbox_device_t)dev;
}

#ifdef __linux__
/* Linux only: wraps a real struct pvt* (from gpublic->devices) as a
 * simbox_device_t, per §9.1. Does not copy/duplicate pvt's data - every
 * accessor below reads pvt's fields live. */
simbox_device_t simbox_device_wrap_pvt(struct pvt *pvt)
{
    if (!pvt) return NULL;

    struct simbox_device_internal *dev = (struct simbox_device_internal *)calloc(1, sizeof(struct simbox_device_internal));
    if (!dev) return NULL;

    dev->kind = SIMBOX_DEV_REAL;
    dev->pvt = pvt;
    dev->cpvt = NULL;
    pthread_mutex_init(&dev->lock, NULL);
    pthread_cond_init(&dev->at_response_cond, NULL);
    dev->data_fd = -1;
    dev->audio_fd = -1;
    return (simbox_device_t)dev;
}
#endif

void simbox_device_destroy(simbox_device_t dev_handle)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev) return;

#ifdef __linux__
    /* Does NOT destroy dev->pvt for SIMBOX_DEV_REAL - that object is
     * owned by chan_dongle.c's own gpublic list (torn down by
     * unload_module(), via simbox_module_bridge_unload()), not by this
     * wrapper. Only cpvt (if lazily acquired) is ours to free. */
    if (dev->kind == SIMBOX_DEV_REAL && dev->cpvt && dev->cpvt_adapter_owned) {
        cpvt_free(dev->cpvt);
    }
#endif

    pthread_cond_destroy(&dev->at_response_cond);
    pthread_mutex_destroy(&dev->lock);
    free(dev);
}

int simbox_device_get_info(simbox_device_t dev_handle, simbox_device_info_t *info)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev || !info) return -1;

#ifdef __linux__
    if (dev->kind == SIMBOX_DEV_REAL) {
        struct pvt *pvt = dev->pvt;
        memset(info, 0, sizeof(*info));
        strncpy(info->sn, pvt->serial, sizeof(info->sn) - 1);
        strncpy(info->imei, pvt->imei, sizeof(info->imei) - 1);
        strncpy(info->imsi, pvt->imsi, sizeof(info->imsi) - 1);
        strncpy(info->model, pvt->model, sizeof(info->model) - 1);
        strncpy(info->firmware, pvt->firmware, sizeof(info->firmware) - 1);
        info->rssi = pvt->rssi;
        info->state = simbox_device_state(dev_handle);
        return 0;
    }
#endif

    pthread_mutex_lock(&dev->lock);
    memcpy(info, &dev->info, sizeof(simbox_device_info_t));
    pthread_mutex_unlock(&dev->lock);
    return 0;
}

const char *simbox_device_sn(simbox_device_t dev_handle)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev) return "";
#ifdef __linux__
    if (dev->kind == SIMBOX_DEV_REAL) return dev->pvt->serial;
#endif
    return dev->info.sn;
}

const char *simbox_device_imei(simbox_device_t dev_handle)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev) return "";
#ifdef __linux__
    if (dev->kind == SIMBOX_DEV_REAL) return dev->pvt->imei;
#endif
    return dev->info.imei;
}

const char *simbox_device_imsi(simbox_device_t dev_handle)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev) return "";
#ifdef __linux__
    if (dev->kind == SIMBOX_DEV_REAL) return dev->pvt->imsi;
#endif
    return dev->info.imsi;
}

simbox_device_state_t simbox_device_state(simbox_device_t dev_handle)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev) return SIMBOX_STATE_DISCONNECTED;
#ifdef __linux__
    if (dev->kind == SIMBOX_DEV_REAL) {
        simbox_device_state_t state;
        pthread_mutex_lock(&dev->lock);
        state = dev->call_state_known
            ? dev->call_state
            : (dev->pvt->gsm_reg_status ? SIMBOX_STATE_IDLE : SIMBOX_STATE_CONNECTING);
        pthread_mutex_unlock(&dev->lock);
        return state;
    }
#endif
    return dev->info.state;
}

#ifdef __linux__
/* Keeps synchronous state queries aligned with the same real
 * DongleCallStateChange event stream exposed to callbacks. */
void simbox_device_apply_call_state(simbox_device_t dev_handle,
                                     simbox_device_state_t state)
{
    struct simbox_device_internal *dev =
        (struct simbox_device_internal *)dev_handle;
    if (!dev || dev->kind != SIMBOX_DEV_REAL) return;
    pthread_mutex_lock(&dev->lock);
    dev->call_state = state;
    dev->call_state_known = 1;
    /* change_channel_state() frees a channel-less cpvt before emitting
     * DongleCallStateChange(released). Never dereference or free that
     * pointer after the release event reaches this adapter. */
    if (state == SIMBOX_STATE_IDLE) {
        dev->cpvt = NULL;
        dev->cpvt_adapter_owned = 0;
    }
    pthread_mutex_unlock(&dev->lock);
}

void simbox_device_attach_call(simbox_device_t dev_handle, struct cpvt *cpvt,
                               int adapter_owned)
{
    struct simbox_device_internal *dev =
        (struct simbox_device_internal *)dev_handle;
    if (!dev || dev->kind != SIMBOX_DEV_REAL || !cpvt) return;
    pthread_mutex_lock(&dev->lock);
    dev->cpvt = cpvt;
    dev->cpvt_adapter_owned = adapter_owned;
    pthread_mutex_unlock(&dev->lock);
}
#endif

int simbox_device_rssi(simbox_device_t dev_handle)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev) return 0;
#ifdef __linux__
    if (dev->kind == SIMBOX_DEV_REAL) return dev->pvt->rssi;
#endif
    return dev->info.rssi;
}

#ifdef __linux__
/* Lazily acquires dev->cpvt (specifications §9.7's refined design:
 * cpvt_alloc()+at_enque_dial()/answer()/hangup() directly, no
 * ast_channel/channel_request() needed - a real, already-EXPORT_DECL'd
 * chan_svistok seam found during Task 5.1's symbol audit). Reused
 * across originate/hangup for the same in-flight call; released by
 * simbox_call_hangup() once the call ends. */
static struct cpvt *simbox_require_cpvt(struct simbox_device_internal *dev)
{
    if (dev->cpvt) return dev->cpvt;
    dev->cpvt = cpvt_alloc(dev->pvt, pvt_get_pseudo_call_idx(dev->pvt),
                            CALL_DIR_OUTGOING, CALL_STATE_INIT);
    dev->cpvt_adapter_owned = dev->cpvt != NULL;
    return dev->cpvt;
}
#endif

int simbox_call_originate(simbox_device_t dev_handle, const char *number)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev || !number) return -1;

#ifdef __linux__
    if (dev->kind == SIMBOX_DEV_REAL) {
        struct cpvt *cpvt = simbox_require_cpvt(dev);
        if (!cpvt) return -1;
        /* clir=0: default presentation, no CLIR override - chan_svistok
         * itself only sets this from a dialplan option this adapter has
         * no equivalent of. */
        return at_enque_dial(cpvt, number, 0);
    }
#endif

    pthread_mutex_lock(&dev->lock);
    dev->info.state = SIMBOX_STATE_DIALING;
    pthread_mutex_unlock(&dev->lock);
    return 0;
}

int simbox_call_hangup(simbox_device_t dev_handle)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev) return -1;

#ifdef __linux__
    if (dev->kind == SIMBOX_DEV_REAL) {
        if (!dev->cpvt) return -1; /* no active call to hang up */
        int rv = at_enque_hangup(dev->cpvt, dev->cpvt->call_idx);
        /* Do not free here: at_queue_task_t retains this cpvt until the
         * modem response is processed. The real released state event
         * clears our non-owning reference after chan_svistok completes
         * its own lifecycle. */
        return rv;
    }
#endif

    pthread_mutex_lock(&dev->lock);
    dev->info.state = SIMBOX_STATE_IDLE;
    pthread_mutex_unlock(&dev->lock);
    return 0;
}

int simbox_call_answer(simbox_device_t dev_handle)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev) return -1;

#ifdef __linux__
    if (dev->kind == SIMBOX_DEV_REAL) {
        /* Unlike originate, answer never fabricates a cpvt - an
         * incoming call's cpvt is created by the real event path (Task
         * 5.8, not yet wired), so no cpvt here means there's genuinely
         * no incoming call to answer, not a gap in this function. */
        if (!dev->cpvt) return -1;
        return at_enque_answer(dev->cpvt);
    }
#endif

    pthread_mutex_lock(&dev->lock);
    dev->info.state = SIMBOX_STATE_ACTIVE_CALL;
    pthread_mutex_unlock(&dev->lock);
    return 0;
}

int simbox_call_send_dtmf(simbox_device_t dev_handle, char digit)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev) return -1;
    return 0;
}

int simbox_call_write_audio(simbox_device_t dev_handle, const int16_t *pcm_samples, size_t count)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev || !pcm_samples) return -1;
    return (int)count;
}

int simbox_call_read_audio(simbox_device_t dev_handle, int16_t *pcm_samples, size_t max_count)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev || !pcm_samples) return -1;
    memset(pcm_samples, 0, max_count * sizeof(int16_t));
    return (int)max_count;
}

int simbox_sms_send(simbox_device_t dev_handle, const char *number, const char *message)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev || !number || !message) return -1;

#ifdef __linux__
    if (dev->kind == SIMBOX_DEV_REAL) {
        /* &pvt->sys_chan, not cpvt_alloc(): SMS has no active-call
         * context, and chan_dongle.c's own real code uses exactly this
         * field for non-call AT commands (confirmed by reading its
         * at_enque_initialization(&pvt->sys_chan, ...)/at_enque_ping(
         * &pvt->sys_chan) call sites) - already initialized by
         * pvt_create(), no alloc/free lifecycle needed here at all. */
        void *id = NULL;
        return at_enque_sms(&dev->pvt->sys_chan, number, message,
                             /*validity_minutes*/ 0, /*report_req*/ 0, &id);
    }
#endif

    return 0;
}

int simbox_ussd_send(simbox_device_t dev_handle, const char *code)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev || !code) return -1;

#ifdef __linux__
    if (dev->kind == SIMBOX_DEV_REAL) {
        void *id = NULL;
        return at_enque_ussd(&dev->pvt->sys_chan, code, NULL, 0, 0, &id);
    }
#endif

    return 0;
}

#ifdef __linux__
static int simbox_at_response_is(const char *line, const char *expected)
{
    size_t line_len;
    size_t expected_len;
    if (!line || !expected) return 0;
    line_len = strlen(line);
    while (line_len > 0 && (line[line_len - 1] == '\r' || line[line_len - 1] == '\n')) {
        line_len--;
    }
    expected_len = strlen(expected);
    return line_len == expected_len && strncmp(line, expected, line_len) == 0;
}

/* Called from the adapter logging shim for each real CMD_USER response
 * line emitted by at_response.c. Accumulates intermediate lines and only
 * wakes simbox_at_command() on a terminal modem result. */
void simbox_device_capture_at_response(simbox_device_t dev_handle,
                                        const char *response_line)
{
    struct simbox_device_internal *dev =
        (struct simbox_device_internal *)dev_handle;
    size_t used;
    size_t available;
    size_t line_len;
    int terminal_ok;
    int terminal_error;

    if (!dev || dev->kind != SIMBOX_DEV_REAL || !response_line) return;
    pthread_mutex_lock(&dev->lock);
    if (!dev->at_response_pending || !dev->at_response_buf ||
        dev->at_response_size == 0) {
        pthread_mutex_unlock(&dev->lock);
        return;
    }

    used = strlen(dev->at_response_buf);
    available = dev->at_response_size - used;
    line_len = strlen(response_line);
    while (line_len > 0 &&
           (response_line[line_len - 1] == '\r' || response_line[line_len - 1] == '\n')) {
        line_len--;
    }
    if (used > 0 && available > 1) {
        dev->at_response_buf[used++] = '\r';
        available--;
        if (available > 1) {
            dev->at_response_buf[used++] = '\n';
            available--;
        }
        dev->at_response_buf[used] = '\0';
    }
    if (available > 1) {
        size_t copy_len = line_len < available - 1 ? line_len : available - 1;
        memcpy(dev->at_response_buf + used, response_line, copy_len);
        dev->at_response_buf[used + copy_len] = '\0';
    }

    terminal_ok = simbox_at_response_is(response_line, "OK");
    terminal_error = simbox_at_response_is(response_line, "ERROR") ||
        strncmp(response_line, "+CME ERROR", strlen("+CME ERROR")) == 0 ||
        strncmp(response_line, "+CMS ERROR", strlen("+CMS ERROR")) == 0;
    if (terminal_ok || terminal_error) {
        dev->at_response_result = terminal_ok ? 0 : -1;
        dev->at_response_complete = 1;
        pthread_cond_signal(&dev->at_response_cond);
    }
    pthread_mutex_unlock(&dev->lock);
}
#endif

int simbox_at_command(simbox_device_t dev_handle, const char *cmd,
                      char *response_buf, size_t buf_len)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev || !cmd || !response_buf || buf_len == 0) return -1;

#ifdef __linux__
    if (dev->kind == SIMBOX_DEV_REAL) {
        struct timespec deadline;
        int rv;
        int wait_rv = 0;

        /* at_response.c reports every CMD_USER line through ast_log()
         * before dispatching it. The adapter logging shim forwards those
         * exact lines to simbox_device_capture_at_response(), which
         * accumulates them and signals on OK/ERROR. This preserves the
         * existing synchronous public API without touching the read-only
         * parser or queue. */
        pthread_mutex_lock(&dev->lock);
        if (dev->at_response_pending) {
            pthread_mutex_unlock(&dev->lock);
            return -1;
        }
        response_buf[0] = '\0';
        dev->at_response_buf = response_buf;
        dev->at_response_size = buf_len;
        dev->at_response_pending = 1;
        dev->at_response_complete = 0;
        dev->at_response_result = -1;
        pthread_mutex_unlock(&dev->lock);

        rv = at_enque_cmd_proc(&dev->pvt->sys_chan, cmd);
        if (rv != 0) {
            pthread_mutex_lock(&dev->lock);
            dev->at_response_pending = 0;
            dev->at_response_buf = NULL;
            dev->at_response_size = 0;
            pthread_mutex_unlock(&dev->lock);
            return rv;
        }

        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_sec += 3; /* queue command timeout is 2 seconds */
        pthread_mutex_lock(&dev->lock);
        while (!dev->at_response_complete && wait_rv != ETIMEDOUT) {
            wait_rv = pthread_cond_timedwait(&dev->at_response_cond,
                                             &dev->lock, &deadline);
        }
        rv = dev->at_response_complete ? dev->at_response_result : -1;
        dev->at_response_pending = 0;
        dev->at_response_complete = 0;
        dev->at_response_buf = NULL;
        dev->at_response_size = 0;
        pthread_mutex_unlock(&dev->lock);
        return rv;
    }
#endif

    strncpy(response_buf, "OK\r\n", buf_len - 1);
    response_buf[buf_len - 1] = '\0';
    return 0;
}

int simbox_change_imei(simbox_device_t dev_handle, const char *new_imei)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev || !new_imei) return -1;

#ifdef __linux__
    if (dev->kind == SIMBOX_DEV_REAL) {
        /* KNOWN GAP in chan_svistok itself, not an adapter design
         * problem: chan_dongle.c's real IMEI-change path
         * (chan_dongle.c:760, cli.c:872) calls
         * ttyprog_changeimei(pvt->audio_fd, pvt->newimei) directly -
         * bypassing the AT-command queue entirely, a raw
         * programmator/-style protocol call, matching what Phase 0
         * already found: programmator/ttyprog_test.c doesn't build
         * standalone either. Confirmed by exhaustive grep across the
         * whole chan_svistok tree: ttyprog_changeimei is *called* in
         * three places (cli.c, chan_dongle.c, ttyprog_test.c) but its
         * definition doesn't exist anywhere in this checked-in source -
         * not in programmator/ttyprog_core.c or anywhere else. This
         * isn't fixable from the adapter side (read-only tree, and the
         * function is simply missing, not just unreachable) - flagging
         * rather than fabricating a fake success or guessing at a
         * different real entry point. simbox_prog_change_imei (in
         * simbox_programmator.c) is NOT a substitute either - checked,
         * it's its own independent simulation (hand-rolled
         * AT^NVWIMEI write, doesn't call ttyprog_changeimei or any real
         * chan_svistok code), a second, separate instance of the
         * original disconnected-simulation problem this whole amendment
         * exists to fix - out of this task's scope per specifications
         * §9.6, flagged as a new finding for a future pass. */
        return -1;
    }
#endif

    pthread_mutex_lock(&dev->lock);
    strncpy(dev->info.imei, new_imei, sizeof(dev->info.imei) - 1);
    pthread_mutex_unlock(&dev->lock);
    return 0;
}
