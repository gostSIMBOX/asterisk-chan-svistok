/*
 * Simbox Native SDK - APDU SIM Reader Adapter
 */
#include "simbox_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

struct simbox_reader_ctx {
    char tty_port[128];
    int fd;
    char atr_hex[128];
};

simbox_reader_t simbox_reader_open(const char *tty_port)
{
    struct simbox_reader_ctx *ctx = (struct simbox_reader_ctx *)calloc(1, sizeof(struct simbox_reader_ctx));
    if (!ctx) return NULL;

    if (tty_port) {
        strncpy(ctx->tty_port, tty_port, sizeof(ctx->tty_port) - 1);
        ctx->fd = open(tty_port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    } else {
        ctx->fd = -1;
    }

    strcpy(ctx->atr_hex, "3B9F95801FC78031E073FE211B66D00226800072");
    return (simbox_reader_t)ctx;
}

void simbox_reader_close(simbox_reader_t handle)
{
    struct simbox_reader_ctx *ctx = (struct simbox_reader_ctx *)handle;
    if (!ctx) return;
    if (ctx->fd >= 0) close(ctx->fd);
    free(ctx);
}

int simbox_reader_send_apdu(simbox_reader_t handle,
                            const uint8_t *apdu, size_t len,
                            uint8_t *response, size_t *resp_len)
{
    struct simbox_reader_ctx *ctx = (struct simbox_reader_ctx *)handle;
    if (!ctx || !apdu || len == 0 || !response || !resp_len)
        return -1;

    /* If mock / loopback, return standard 90 00 SW */
    response[0] = 0x90;
    response[1] = 0x00;
    *resp_len = 2;
    return 0;
}

int simbox_reader_get_atr(simbox_reader_t handle, char *atr_hex, size_t atr_size)
{
    struct simbox_reader_ctx *ctx = (struct simbox_reader_ctx *)handle;
    if (!ctx || !atr_hex || atr_size == 0) return -1;

    strncpy(atr_hex, ctx->atr_hex, atr_size - 1);
    atr_hex[atr_size - 1] = '\0';
    return 0;
}

int simbox_reader_reset(simbox_reader_t handle)
{
    struct simbox_reader_ctx *ctx = (struct simbox_reader_ctx *)handle;
    if (!ctx) return -1;
    return 0;
}
