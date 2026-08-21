/*
 * Asterisk compatibility shim for chan_simbox
 * timing.h - Timing interface (timerfd wrappers on Linux)
 */
#ifndef ASTERISK_TIMING_H
#define ASTERISK_TIMING_H

struct ast_timer;

#ifdef __cplusplus
extern "C" {
#endif

struct ast_timer *ast_timer_open(void);
void ast_timer_close(struct ast_timer *handle);
int ast_timer_fd(const struct ast_timer *handle);
int ast_timer_set_rate(struct ast_timer *handle, unsigned int rate);
void ast_timer_ack(struct ast_timer *handle, unsigned int quantity);

#ifdef __cplusplus
}
#endif

#endif /* ASTERISK_TIMING_H */
