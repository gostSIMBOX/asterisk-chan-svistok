/*
 * Asterisk compatibility shim for chan_simbox
 * frame.h - Media frames & control signals
 */
#ifndef ASTERISK_FRAME_H
#define ASTERISK_FRAME_H

#include <asterisk/format.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define AST_FRIENDLY_OFFSET 64

enum ast_frame_type {
    AST_FRAME_NULL = 0,
    AST_FRAME_DTMF,
    AST_FRAME_VOICE,
    AST_FRAME_VIDEO,
    AST_FRAME_CONTROL,
    AST_FRAME_NULL_CNG,
    AST_FRAME_MODEM,
    AST_FRAME_DTMF_BEGIN,
    AST_FRAME_DTMF_END,
    AST_FRAME_IMAGE,
    AST_FRAME_HTML,
    AST_FRAME_CNG,
};

enum ast_control_frame_type {
    AST_CONTROL_HANGUP = 1,
    AST_CONTROL_RING = 2,
    AST_CONTROL_RINGING = 3,
    AST_CONTROL_ANSWER = 4,
    AST_CONTROL_BUSY = 5,
    AST_CONTROL_TAKEOFFHOOK = 6,
    AST_CONTROL_OFFHOOK = 7,
    AST_CONTROL_CONGESTION = 8,
    AST_CONTROL_FLASH = 9,
    AST_CONTROL_WINK = 10,
    AST_CONTROL_OPTION = 11,
    AST_CONTROL_RADIO_KEY = 12,
    AST_CONTROL_RADIO_UNKEY = 13,
    AST_CONTROL_PROGRESS = 14,
    AST_CONTROL_PROCEEDING = 15,
    AST_CONTROL_HOLD = 16,
    AST_CONTROL_UNHOLD = 17,
    AST_CONTROL_VIDUPDATE = 18,
    AST_CONTROL_SRCUPDATE = 19,
    AST_CONTROL_TRANSFER = 20,
    AST_CONTROL_CONNECTED_LINE = 21,
    AST_CONTROL_REDIRECTING = 22,
    AST_CONTROL_T38_PARAMETERS = 23,
    AST_CONTROL_CC = 24,
    AST_CONTROL_SRCCHANGE = 25,
    AST_CONTROL_READ_ACTION = 26,
    AST_CONTROL_AOC = 27,
    AST_CONTROL_END_OF_Q = 28,
    AST_CONTROL_INCOMPLETE = 29,
    AST_CONTROL_MCID = 30,
    AST_CONTROL_UPDATE_RTP_PEER = 31,
};

#define AST_FRAME_MALLOCD_HDR    (1 << 0)
#define AST_FRAME_MALLOCD_SRC    (1 << 1)
#define AST_FRAME_MALLOCD_DATA   (1 << 2)

struct ast_frame {
    enum ast_frame_type frametype;
    union {
        int integer;
        struct ast_format format;
    } subclass;
    #define subclass_integer subclass.integer

    int datalen;
    int samples;
    int mallocd;
    int offset;
    int len; /* Length in ms */
    const char *src;
    union {
        void *ptr;
        uint32_t uint32;
        char pad[8];
    } data;
    struct {
        struct ast_frame *next;
    } frame_list;
};

extern struct ast_frame ast_null_frame;

#define ast_frame_byteswap_le(f) do { } while (0)

static inline int ast_frame_adjust_volume(struct ast_frame *f, int step)
{
    return 0;
}

static inline void ast_frfree(struct ast_frame *fr)
{
    if (!fr) return;
    if (fr->mallocd & AST_FRAME_MALLOCD_DATA) {
        if (fr->data.ptr) free(fr->data.ptr);
    }
    if (fr->mallocd & AST_FRAME_MALLOCD_SRC) {
        if (fr->src) free((void *)fr->src);
    }
    if (fr->mallocd & AST_FRAME_MALLOCD_HDR) {
        free(fr);
    }
}

static inline struct ast_frame *ast_frisolate(struct ast_frame *fr)
{
    if (!fr) return NULL;
    if (!(fr->mallocd & AST_FRAME_MALLOCD_HDR)) {
        struct ast_frame *nfr = (struct ast_frame *)malloc(sizeof(struct ast_frame));
        if (!nfr) return NULL;
        memcpy(nfr, fr, sizeof(struct ast_frame));
        nfr->mallocd = AST_FRAME_MALLOCD_HDR;
        if (fr->datalen > 0 && fr->data.ptr) {
            void *buf = malloc(fr->datalen + AST_FRIENDLY_OFFSET);
            if (!buf) {
                free(nfr);
                return NULL;
            }
            memcpy((char *)buf + AST_FRIENDLY_OFFSET, fr->data.ptr, fr->datalen);
            nfr->data.ptr = (char *)buf + AST_FRIENDLY_OFFSET;
            nfr->offset = AST_FRIENDLY_OFFSET;
            nfr->mallocd |= AST_FRAME_MALLOCD_DATA;
        }
        return nfr;
    }
    return fr;
}

static inline struct ast_frame *ast_frdup(const struct ast_frame *fr)
{
    if (!fr) return NULL;
    struct ast_frame *nfr = (struct ast_frame *)malloc(sizeof(struct ast_frame));
    if (!nfr) return NULL;
    memcpy(nfr, fr, sizeof(struct ast_frame));
    nfr->mallocd = AST_FRAME_MALLOCD_HDR;
    if (fr->datalen > 0 && fr->data.ptr) {
        void *buf = malloc(fr->datalen + AST_FRIENDLY_OFFSET);
        if (!buf) {
            free(nfr);
            return NULL;
        }
        memcpy((char *)buf + AST_FRIENDLY_OFFSET, fr->data.ptr, fr->datalen);
        nfr->data.ptr = (char *)buf + AST_FRIENDLY_OFFSET;
        nfr->offset = AST_FRIENDLY_OFFSET;
        nfr->mallocd |= AST_FRAME_MALLOCD_DATA;
    }
    return nfr;
}

#endif /* ASTERISK_FRAME_H */
