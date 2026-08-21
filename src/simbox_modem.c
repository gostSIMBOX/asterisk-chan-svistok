/*
 * Simbox Native SDK - Modem Device Driver Adapter
 */
#include "simbox_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

struct simbox_device_internal {
    simbox_device_info_t info;
    pthread_mutex_t lock;
    int data_fd;
    int audio_fd;
};

simbox_device_t simbox_device_create(const simbox_device_info_t *info)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)calloc(1, sizeof(struct simbox_device_internal));
    if (!dev) return NULL;

    if (info) {
        memcpy(&dev->info, info, sizeof(simbox_device_info_t));
    }
    pthread_mutex_init(&dev->lock, NULL);
    dev->data_fd = -1;
    dev->audio_fd = -1;
    return (simbox_device_t)dev;
}

void simbox_device_destroy(simbox_device_t dev_handle)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev) return;

    pthread_mutex_destroy(&dev->lock);
    free(dev);
}

int simbox_device_get_info(simbox_device_t dev_handle, simbox_device_info_t *info)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev || !info) return -1;

    pthread_mutex_lock(&dev->lock);
    memcpy(info, &dev->info, sizeof(simbox_device_info_t));
    pthread_mutex_unlock(&dev->lock);
    return 0;
}

const char *simbox_device_sn(simbox_device_t dev_handle)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    return dev ? dev->info.sn : "";
}

const char *simbox_device_imei(simbox_device_t dev_handle)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    return dev ? dev->info.imei : "";
}

const char *simbox_device_imsi(simbox_device_t dev_handle)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    return dev ? dev->info.imsi : "";
}

simbox_device_state_t simbox_device_state(simbox_device_t dev_handle)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    return dev ? dev->info.state : SIMBOX_STATE_DISCONNECTED;
}

int simbox_device_rssi(simbox_device_t dev_handle)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    return dev ? dev->info.rssi : 0;
}

int simbox_call_originate(simbox_device_t dev_handle, const char *number)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev || !number) return -1;

    pthread_mutex_lock(&dev->lock);
    dev->info.state = SIMBOX_STATE_DIALING;
    pthread_mutex_unlock(&dev->lock);
    return 0;
}

int simbox_call_hangup(simbox_device_t dev_handle)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev) return -1;

    pthread_mutex_lock(&dev->lock);
    dev->info.state = SIMBOX_STATE_IDLE;
    pthread_mutex_unlock(&dev->lock);
    return 0;
}

int simbox_call_answer(simbox_device_t dev_handle)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev) return -1;

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
    return 0;
}

int simbox_ussd_send(simbox_device_t dev_handle, const char *code)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev || !code) return -1;
    return 0;
}

int simbox_at_command(simbox_device_t dev_handle, const char *cmd,
                      char *response_buf, size_t buf_len)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev || !cmd || !response_buf || buf_len == 0) return -1;

    strncpy(response_buf, "OK\r\n", buf_len - 1);
    response_buf[buf_len - 1] = '\0';
    return 0;
}

int simbox_change_imei(simbox_device_t dev_handle, const char *new_imei)
{
    struct simbox_device_internal *dev = (struct simbox_device_internal *)dev_handle;
    if (!dev || !new_imei) return -1;

    pthread_mutex_lock(&dev->lock);
    strncpy(dev->info.imei, new_imei, sizeof(dev->info.imei) - 1);
    pthread_mutex_unlock(&dev->lock);
    return 0;
}
