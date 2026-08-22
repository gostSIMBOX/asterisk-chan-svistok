/*
 * Simbox Native SDK - Master API Implementation
 */
#include "simbox_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#ifdef __linux__
#include "simbox_internal_linux.h"
#include "simbox_module_bridge.h"
#include <chan_dongle.h>
#endif

#define SIMBOX_MAX_DEVICES 256

struct simbox_instance {
    simbox_config_t config;
    simbox_event_cb event_cb;
    void *event_userdata;
    pthread_mutex_t lock;
    simbox_device_t devices[SIMBOX_MAX_DEVICES];
    int device_count;
};

static const char *SDK_VERSION = "1.0.0-standalone";

extern simbox_device_t simbox_device_create(const simbox_device_info_t *info);
extern void simbox_device_destroy(simbox_device_t dev);

#ifdef __linux__
/* The real chan_svistok engine (gpublic, load_module) is process-global
 * by chan_dongle.c's own design - there is only ever meaningfully one
 * "real" Linux instance, regardless of how many simbox_handle_t a
 * caller creates. This tracks whichever one is currently active so
 * adapter-side event hooks (shim_pbx.c's ast_pbx_start, etc. - see
 * simbox_event_bridge_fire_incoming_call below) know which event_cb to
 * invoke, without needing simbox_instance's definition to be visible
 * outside this file. Last simbox_init() wins if more than one Linux
 * instance is ever created (not a supported configuration, not
 * guarded against beyond this). */
static struct simbox_instance *g_active_linux_instance = NULL;
#endif

#ifdef __linux__
/* Real path (specifications §9.1): trigger chan_dongle.c's own
 * unmodified, config-file-driven device population via the captured
 * load_module(), then wrap each resulting real struct pvt* from
 * gpublic->devices as a simbox_device_t. Non-fatal on failure (no
 * config file / load_module decline) - inst simply starts with 0
 * devices, same shape as the non-Linux/no-devices-registered-yet
 * baseline, not a crash. */
static void simbox_populate_from_gpublic(struct simbox_instance *inst)
{
    int rv = simbox_module_bridge_load();
    if (rv != 0 || !gpublic) {
        return;
    }

    struct pvt *pvt;
    AST_RWLIST_RDLOCK(&gpublic->devices);
    AST_RWLIST_TRAVERSE(&gpublic->devices, pvt, entry) {
        if (inst->device_count >= SIMBOX_MAX_DEVICES) break;
        simbox_device_t dev = simbox_device_wrap_pvt(pvt);
        if (dev) {
            inst->devices[inst->device_count++] = dev;
        }
    }
    AST_RWLIST_UNLOCK(&gpublic->devices);
}

static char *simbox_event_copy_string(char **cursor, const char *value)
{
    char *copy = *cursor;
    size_t len = value ? strlen(value) : 0;
    if (len) memcpy(copy, value, len);
    copy[len] = '\0';
    *cursor += len + 1;
    return copy;
}

static simbox_event_t *simbox_event_alloc(const char *device_sn,
                                           const char *value1,
                                           const char *value2,
                                           char **copy1,
                                           char **copy2)
{
    size_t device_len = device_sn ? strlen(device_sn) : 0;
    size_t value1_len = value1 ? strlen(value1) : 0;
    size_t value2_len = value2 ? strlen(value2) : 0;
    size_t size = sizeof(simbox_event_t) + device_len + 1;
    char *cursor;
    simbox_event_t *event;

    if (copy1) size += value1_len + 1;
    if (copy2) size += value2_len + 1;
    event = (simbox_event_t *)calloc(1, size);
    if (!event) return NULL;

    cursor = (char *)(event + 1);
    event->device_sn = simbox_event_copy_string(&cursor, device_sn);
    if (copy1) *copy1 = simbox_event_copy_string(&cursor, value1);
    if (copy2) *copy2 = simbox_event_copy_string(&cursor, value2);
    return event;
}

static void simbox_event_dispatch(struct simbox_instance *inst,
                                  simbox_event_t *event)
{
    simbox_event_cb cb;
    void *userdata;

    if (!inst || !event) {
        free(event);
        return;
    }
    pthread_mutex_lock(&inst->lock);
    cb = inst->event_cb;
    userdata = inst->event_userdata;
    pthread_mutex_unlock(&inst->lock);
    if (cb) cb(event, userdata);
    else free(event);
}

static simbox_device_state_t simbox_call_state_from_name(const char *state)
{
    if (!state) return SIMBOX_STATE_ERROR;
    if (strcmp(state, "active") == 0) return SIMBOX_STATE_ACTIVE_CALL;
    if (strcmp(state, "held") == 0) return SIMBOX_STATE_HOLD;
    if (strcmp(state, "dialing") == 0 || strcmp(state, "alerting") == 0)
        return SIMBOX_STATE_DIALING;
    if (strcmp(state, "incoming") == 0 || strcmp(state, "waiting") == 0)
        return SIMBOX_STATE_RINGING;
    if (strcmp(state, "released") == 0) return SIMBOX_STATE_IDLE;
    if (strcmp(state, "initialize") == 0) return SIMBOX_STATE_CONNECTING;
    return SIMBOX_STATE_ERROR;
}

static int simbox_instance_device_index(struct simbox_instance *inst,
                                         const char *device_sn)
{
    int i;
    if (!inst || !device_sn) return -1;
    for (i = 0; i < inst->device_count; i++) {
        const char *candidate = simbox_device_sn(inst->devices[i]);
        if (candidate && strcmp(candidate, device_sn) == 0) return i;
    }
    return -1;
}

/* See simbox_internal_linux.h's doc comment. */
void simbox_event_bridge_fire_incoming_call(const char *device_sn,
                                             const char *caller,
                                             struct cpvt *cpvt)
{
    struct simbox_instance *inst = g_active_linux_instance;
    char *caller_copy;
    simbox_event_t *event;
    int index;

    if (!inst) return;
    pthread_mutex_lock(&inst->lock);
    index = simbox_instance_device_index(inst, device_sn);
    if (index >= 0) {
        simbox_device_attach_call(inst->devices[index], cpvt,
                                  /* adapter_owned */ 0);
    }
    pthread_mutex_unlock(&inst->lock);
    /* One allocation owns the event and all strings. Ownership transfers
     * to cb(), whose documented free(event) therefore releases all of it.
     * See
     * simbox_event_cb's doc comment in simbox_types.h (Task 5.2 found
     * this is required, not optional, for async-safe listener
     * callbacks like Dart's NativeCallable.listener). */
    event = simbox_event_alloc(device_sn, caller, NULL, &caller_copy, NULL);
    if (!event) return;
    event->type = SIMBOX_EVENT_INCOMING_CALL;
    event->data.incoming_call.caller = caller_copy;
    simbox_event_dispatch(inst, event);
}

void simbox_event_bridge_fire_call_state(const char *device_sn,
                                          const char *new_state_name)
{
    struct simbox_instance *inst = g_active_linux_instance;
    simbox_device_state_t new_state;
    simbox_device_state_t old_state = SIMBOX_STATE_DISCONNECTED;
    simbox_event_t *event;
    int index;

    if (!inst) return;
    new_state = simbox_call_state_from_name(new_state_name);

    pthread_mutex_lock(&inst->lock);
    index = simbox_instance_device_index(inst, device_sn);
    if (index >= 0) {
        old_state = simbox_device_state(inst->devices[index]);
        simbox_device_apply_call_state(inst->devices[index], new_state);
    }
    pthread_mutex_unlock(&inst->lock);

    event = simbox_event_alloc(device_sn, NULL, NULL, NULL, NULL);
    if (!event) return;
    event->type = SIMBOX_EVENT_CALL_STATE_CHANGED;
    event->data.call_state.old_state = old_state;
    event->data.call_state.new_state = new_state;
    simbox_event_dispatch(inst, event);
}

void simbox_event_bridge_fire_incoming_sms(const char *device_sn,
                                            const char *sender,
                                            const char *text)
{
    struct simbox_instance *inst = g_active_linux_instance;
    char *sender_copy;
    char *text_copy;
    simbox_event_t *event;

    if (!inst) return;
    event = simbox_event_alloc(device_sn, sender, text,
                               &sender_copy, &text_copy);
    if (!event) return;
    event->type = SIMBOX_EVENT_INCOMING_SMS;
    event->data.incoming_sms.sender = sender_copy;
    event->data.incoming_sms.text = text_copy;
    simbox_event_dispatch(inst, event);
}

void simbox_event_bridge_fire_ussd_response(const char *device_sn,
                                             const char *response)
{
    struct simbox_instance *inst = g_active_linux_instance;
    char *response_copy;
    simbox_event_t *event;

    if (!inst) return;
    event = simbox_event_alloc(device_sn, response, NULL,
                               &response_copy, NULL);
    if (!event) return;
    event->type = SIMBOX_EVENT_USSD_RESPONSE;
    event->data.ussd.response = response_copy;
    simbox_event_dispatch(inst, event);
}

void simbox_at_response_bridge_capture(const char *device_sn,
                                        const char *response_line)
{
    struct simbox_instance *inst = g_active_linux_instance;
    simbox_device_t dev = NULL;
    int index;

    if (!inst || !device_sn || !response_line) return;
    pthread_mutex_lock(&inst->lock);
    index = simbox_instance_device_index(inst, device_sn);
    if (index >= 0) dev = inst->devices[index];
    pthread_mutex_unlock(&inst->lock);
    if (dev) simbox_device_capture_at_response(dev, response_line);
}
#endif

simbox_handle_t simbox_init(const simbox_config_t *config)
{
    struct simbox_instance *inst = (struct simbox_instance *)calloc(1, sizeof(struct simbox_instance));
    if (!inst) return NULL;

    if (config) {
        memcpy(&inst->config, config, sizeof(simbox_config_t));
    }
    pthread_mutex_init(&inst->lock, NULL);
    inst->device_count = 0;

#ifdef __linux__
    simbox_populate_from_gpublic(inst);
    g_active_linux_instance = inst;
#endif
    /* Non-Linux: inst stays at 0 devices here - chan_svistok is
     * Linux-only by design (see specifications §9's platform-split
     * note), simbox_device_register() remains the only population path
     * on these platforms (also used for tests on any platform). */

    return (simbox_handle_t)inst;
}

void simbox_shutdown(simbox_handle_t handle)
{
    struct simbox_instance *inst = (struct simbox_instance *)handle;
    if (!inst) return;

#ifdef __linux__
    if (g_active_linux_instance == inst) {
        g_active_linux_instance = NULL;
    }
#endif

    pthread_mutex_lock(&inst->lock);
    for (int i = 0; i < inst->device_count; i++) {
        if (inst->devices[i]) {
            simbox_device_destroy(inst->devices[i]);
            inst->devices[i] = NULL;
        }
    }
    inst->device_count = 0;
    pthread_mutex_unlock(&inst->lock);

    pthread_mutex_destroy(&inst->lock);
    free(inst);
}

const char *simbox_version(void)
{
    return SDK_VERSION;
}

void simbox_set_event_callback(simbox_handle_t handle,
                              simbox_event_cb cb, void *userdata)
{
    struct simbox_instance *inst = (struct simbox_instance *)handle;
    if (!inst) return;

    pthread_mutex_lock(&inst->lock);
    inst->event_cb = cb;
    inst->event_userdata = userdata;
    pthread_mutex_unlock(&inst->lock);
}

int simbox_device_count(simbox_handle_t handle)
{
    struct simbox_instance *inst = (struct simbox_instance *)handle;
    return inst ? inst->device_count : 0;
}

simbox_device_t simbox_device_get_by_index(simbox_handle_t handle, int index)
{
    struct simbox_instance *inst = (struct simbox_instance *)handle;
    if (!inst || index < 0 || index >= inst->device_count)
        return NULL;

    return inst->devices[index];
}

simbox_device_t simbox_device_get_by_sn(simbox_handle_t handle, const char *sn)
{
    struct simbox_instance *inst = (struct simbox_instance *)handle;
    if (!inst || !sn) return NULL;

    pthread_mutex_lock(&inst->lock);
    for (int i = 0; i < inst->device_count; i++) {
        if (inst->devices[i]) {
            const char *dev_sn = simbox_device_sn(inst->devices[i]);
            if (dev_sn && strcmp(dev_sn, sn) == 0) {
                pthread_mutex_unlock(&inst->lock);
                return inst->devices[i];
            }
        }
    }
    pthread_mutex_unlock(&inst->lock);
    return NULL;
}

int simbox_device_register(simbox_handle_t handle,
                            const simbox_discovered_device_t *discovered)
{
    struct simbox_instance *inst = (struct simbox_instance *)handle;
    if (!inst || !discovered) return -1;

    /* Idempotent: already-registered serials are a no-op success, not a
     * duplicate entry — simbox_device_get_by_sn() takes the lock itself,
     * so check before acquiring it below. */
    if (simbox_device_get_by_sn(handle, discovered->serial_number)) {
        return 0;
    }

    pthread_mutex_lock(&inst->lock);
    if (inst->device_count >= SIMBOX_MAX_DEVICES) {
        pthread_mutex_unlock(&inst->lock);
        return -1;
    }

    simbox_device_info_t info;
    memset(&info, 0, sizeof(info));
    strncpy(info.sn, discovered->serial_number, sizeof(info.sn) - 1);
    strncpy(info.imei, discovered->imei, sizeof(info.imei) - 1);
    strncpy(info.name, discovered->dev_name, sizeof(info.name) - 1);
    strncpy(info.tty_data, discovered->data_port, sizeof(info.tty_data) - 1);
    strncpy(info.tty_audio, discovered->audio_port, sizeof(info.tty_audio) - 1);
    /* imsi/model/firmware/rssi are unknown at discovery time — populated
     * later once the modem driver queries the device over AT commands,
     * not this function's concern. */
    info.state = SIMBOX_STATE_CONNECTING;

    simbox_device_t dev = simbox_device_create(&info);
    if (!dev) {
        pthread_mutex_unlock(&inst->lock);
        return -1;
    }

    inst->devices[inst->device_count++] = dev;

    simbox_event_cb cb = inst->event_cb;
    void *userdata = inst->event_userdata;
    pthread_mutex_unlock(&inst->lock);

    if (cb) {
        /* Heap-allocated, not stack-local: FFI listener callbacks (e.g.
         * Dart's NativeCallable.listener) are asynchronous — cb()
         * returns before the receiving side actually reads the event,
         * so a stack-local struct would be read after this frame is
         * gone. The callback takes ownership and must free() it once
         * done — see simbox_event_cb's doc comment in simbox_types.h. */
        simbox_event_t *event = (simbox_event_t *)calloc(1, sizeof(simbox_event_t));
        if (event) {
            event->type = SIMBOX_EVENT_DEVICE_CONNECTED;
            event->device_sn = simbox_device_sn(dev);
            cb(event, userdata);
        }
    }

    return 0;
}
