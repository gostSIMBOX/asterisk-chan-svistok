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
