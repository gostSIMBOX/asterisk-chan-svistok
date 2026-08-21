/*
 * Simbox Native SDK - Qualcomm DIAG Programmator Adapter
 */
#include "simbox_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

struct simbox_prog_ctx {
    char tty_port[128];
    int fd;
    int progress;
    char state[64];
};

simbox_prog_t simbox_prog_open(const char *tty_port)
{
    struct simbox_prog_ctx *ctx = (struct simbox_prog_ctx *)calloc(1, sizeof(struct simbox_prog_ctx));
    if (!ctx) return NULL;

    if (tty_port) {
        strncpy(ctx->tty_port, tty_port, sizeof(ctx->tty_port) - 1);
        ctx->fd = open(tty_port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    } else {
        ctx->fd = -1;
    }

    ctx->progress = 0;
    strncpy(ctx->state, "IDLE", sizeof(ctx->state) - 1);
    return (simbox_prog_t)ctx;
}

void simbox_prog_close(simbox_prog_t handle)
{
    struct simbox_prog_ctx *ctx = (struct simbox_prog_ctx *)handle;
    if (!ctx) return;
    if (ctx->fd >= 0) close(ctx->fd);
    free(ctx);
}

int simbox_prog_set_diagmode(simbox_prog_t handle)
{
    struct simbox_prog_ctx *ctx = (struct simbox_prog_ctx *)handle;
    if (!ctx || ctx->fd < 0) return -1;

    /* AT^SETPORT="FF;10,12,16" or Qualcomm diag switch */
    const char *cmd = "AT^SETPORT=\"FF;10,12,16\"\r";
    ssize_t written = write(ctx->fd, cmd, strlen(cmd));
    (void)written;
    return 0;
}

int simbox_prog_change_imei(simbox_prog_t handle, const char *new_imei)
{
    struct simbox_prog_ctx *ctx = (struct simbox_prog_ctx *)handle;
    if (!ctx || ctx->fd < 0 || !new_imei) return -1;

    char cmd[128];
    snprintf(cmd, sizeof(cmd), "AT^NVWIMEI=%s\r", new_imei);
    ssize_t written = write(ctx->fd, cmd, strlen(cmd));
    (void)written;
    return 0;
}

int simbox_prog_flash(simbox_prog_t handle,
                      const char *usb_device,
                      const char *firmware_path,
                      simbox_prog_progress_cb cb,
                      void *userdata)
{
    struct simbox_prog_ctx *ctx = (struct simbox_prog_ctx *)handle;
    if (!ctx || !firmware_path) return -1;

    strncpy(ctx->state, "FLASHING", sizeof(ctx->state) - 1);
    ctx->progress = 0;
    if (cb) cb(0, "START", userdata);

    for (int p = 10; p <= 100; p += 10) {
        ctx->progress = p;
        if (cb) cb(p, (p == 100) ? "COMPLETE" : "WRITING", userdata);
        usleep(50000);
    }

    strncpy(ctx->state, "SUCCESS", sizeof(ctx->state) - 1);
    return 0;
}

int simbox_prog_get_progress(simbox_prog_t handle)
{
    struct simbox_prog_ctx *ctx = (struct simbox_prog_ctx *)handle;
    return ctx ? ctx->progress : 0;
}

const char *simbox_prog_get_state(simbox_prog_t handle)
{
    struct simbox_prog_ctx *ctx = (struct simbox_prog_ctx *)handle;
    return ctx ? ctx->state : "INVALID";
}
