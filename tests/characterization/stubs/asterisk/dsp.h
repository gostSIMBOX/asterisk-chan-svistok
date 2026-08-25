#ifndef SVISTOK_CHARACTERIZATION_ASTERISK_DSP_H
#define SVISTOK_CHARACTERIZATION_ASTERISK_DSP_H
struct ast_dsp;
struct ast_channel;
struct ast_frame;
struct ast_frame *ast_dsp_process(struct ast_channel *, struct ast_dsp *, struct ast_frame *);
#define DSP_DIGITMODE_DTMF 1
#define DSP_DIGITMODE_RELAXDTMF 2
#define DSP_FEATURE_DIGIT_DETECT 4
#define DSP_PROGRESS_TALK 8
#define DSP_PROGRESS_RINGING 16
struct ast_dsp *ast_dsp_new(void);
#endif
