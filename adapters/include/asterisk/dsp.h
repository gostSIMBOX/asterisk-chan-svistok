/*
 * Asterisk compatibility shim for chan_simbox
 * dsp.h - Digital Signal Processing (DTMF, busy detection, silence)
 */
#ifndef ASTERISK_DSP_H
#define ASTERISK_DSP_H

#include <asterisk/frame.h>
#include <asterisk/channel.h>

#define DSP_FEATURE_SILENCE_SUPPRESS (1 << 0)
#define DSP_FEATURE_BUSY_DETECT      (1 << 1)
#define DSP_FEATURE_DIGIT_DETECT     (1 << 2)
#define DSP_FEATURE_FAX_DETECT       (1 << 3)
#define DSP_FEATURE_CALL_PROGRESS    (1 << 4)
#define DSP_FEATURE_WAITDIALTONE     (1 << 5)

#define DSP_DIGITMODE_DTMF           (1 << 0)
#define DSP_DIGITMODE_MF             (1 << 1)
#define DSP_DIGITMODE_RELAX          (1 << 2)
#define DSP_DIGITMODE_RELAXDTMF      (1 << 2)
#define DSP_DIGITMODE_NOQUELCH       (1 << 3)
#define DSP_DIGITMODE_MUTECONF       (1 << 4)
#define DSP_DIGITMODE_MUTEMAX        (1 << 5)

#define DSP_PROGRESS_TALK            (1 << 16)
#define DSP_PROGRESS_RINGING         (1 << 17)

#define DSP_FAXMODE_DETECT_CNG       (1 << 0)
#define DSP_FAXMODE_DETECT_CED       (1 << 1)
#define DSP_FAXMODE_DETECT_SQUELCH   (1 << 2)

#define DSP_TONE_STATE_SILENCE       0
#define DSP_TONE_STATE_RINGING       1
#define DSP_TONE_STATE_DIALTONE      2
#define DSP_TONE_STATE_TALKING       3
#define DSP_TONE_STATE_BUSY          4
#define DSP_TONE_STATE_SPECIAL1      5
#define DSP_TONE_STATE_SPECIAL2      6
#define DSP_TONE_STATE_SPECIAL3      7
#define DSP_TONE_STATE_HUNGUP        8

enum threshold {
    THRESHOLD_SILENCE = 0,
    THRESHOLD_MAX = 1
};

struct ast_dsp;

struct ast_dsp_busy_pattern {
    int length;
    int pattern[4];
};

#ifdef __cplusplus
extern "C" {
#endif

struct ast_dsp *ast_dsp_new(void);
struct ast_dsp *ast_dsp_new_with_rate(unsigned int sample_rate);
void ast_dsp_free(struct ast_dsp *dsp);
void ast_dsp_set_features(struct ast_dsp *dsp, int features);
void ast_dsp_set_threshold(struct ast_dsp *dsp, int threshold);
void ast_dsp_set_busy_count(struct ast_dsp *dsp, int cadences);
void ast_dsp_set_busy_pattern(struct ast_dsp *dsp, const struct ast_dsp_busy_pattern *cadence);
void ast_dsp_digitreset(struct ast_dsp *dsp);
void ast_dsp_reset(struct ast_dsp *dsp);
int ast_dsp_set_digitmode(struct ast_dsp *dsp, int digitmode);
int ast_dsp_set_faxmode(struct ast_dsp *dsp, int faxmode);
int ast_dsp_set_call_progress_zone(struct ast_dsp *dsp, char *zone);
int ast_dsp_was_muted(struct ast_dsp *dsp);
int ast_dsp_get_tstate(struct ast_dsp *dsp);
int ast_dsp_get_tcount(struct ast_dsp *dsp);
int ast_dsp_busydetect(struct ast_dsp *dsp);
int ast_dsp_silence(struct ast_dsp *dsp, struct ast_frame *f, int *totalsilence);
int ast_dsp_silence_with_energy(struct ast_dsp *dsp, struct ast_frame *f, int *totalsilence, int *frames_energy);
int ast_dsp_noise(struct ast_dsp *dsp, struct ast_frame *f, int *totalnoise);
struct ast_frame *ast_dsp_process(struct ast_channel *chan, struct ast_dsp *dsp, struct ast_frame *af);
int a_dsp_call_progress(struct ast_dsp *dsp, struct ast_frame *inf);
int ast_dsp_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ASTERISK_DSP_H */
