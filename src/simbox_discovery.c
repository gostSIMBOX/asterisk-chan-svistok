/*
 * Simbox Native SDK - Discovery Adapter
 */
#include "simbox_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct simbox_discovery_ctx {
    char config_path[512];
    simbox_discovered_device_t devices[128];
    int count;
};

simbox_discovery_t simbox_discovery_start(const char *config_path)
{
    struct simbox_discovery_ctx *ctx = (struct simbox_discovery_ctx *)calloc(1, sizeof(struct simbox_discovery_ctx));
    if (!ctx) return NULL;

    if (config_path) {
        strncpy(ctx->config_path, config_path, sizeof(ctx->config_path) - 1);
    }
    simbox_discovery_scan((simbox_discovery_t)ctx);
    return (simbox_discovery_t)ctx;
}

void simbox_discovery_stop(simbox_discovery_t handle)
{
    if (!handle) return;
    free(handle);
}

int simbox_discovery_scan(simbox_discovery_t handle)
{
    struct simbox_discovery_ctx *ctx = (struct simbox_discovery_ctx *)handle;
    if (!ctx) return -1;

    /* Scan /sys/bus/usb/devices on Linux or simulate on other platforms */
    ctx->count = 0;

    return ctx->count;
}

int simbox_discovery_device_count(simbox_discovery_t handle)
{
    struct simbox_discovery_ctx *ctx = (struct simbox_discovery_ctx *)handle;
    return ctx ? ctx->count : 0;
}

int simbox_discovery_device_get(simbox_discovery_t handle, int index,
                               simbox_discovered_device_t *device)
{
    struct simbox_discovery_ctx *ctx = (struct simbox_discovery_ctx *)handle;
    if (!ctx || !device || index < 0 || index >= ctx->count)
        return -1;

    memcpy(device, &ctx->devices[index], sizeof(simbox_discovered_device_t));
    return 0;
}
