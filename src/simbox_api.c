/*
 * Simbox Native SDK - Master API Implementation
 */
#include "simbox_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

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

simbox_handle_t simbox_init(const simbox_config_t *config)
{
    struct simbox_instance *inst = (struct simbox_instance *)calloc(1, sizeof(struct simbox_instance));
    if (!inst) return NULL;

    if (config) {
        memcpy(&inst->config, config, sizeof(simbox_config_t));
    }
    pthread_mutex_init(&inst->lock, NULL);
    inst->device_count = 0;

    return (simbox_handle_t)inst;
}

void simbox_shutdown(simbox_handle_t handle)
{
    struct simbox_instance *inst = (struct simbox_instance *)handle;
    if (!inst) return;

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
