/*
 * Asterisk compatibility shim for chan_simbox
 * shim_timer.c - Timer abstraction (Linux timerfd with portable fallback)
 */
#include <asterisk/timing.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#if defined(__linux__)
#include <sys/timerfd.h>
#else
#include <pthread.h>
#include <sys/time.h>
#endif

struct ast_timer {
    int fd;
    int rate;
#if !defined(__linux__)
    int pipefd[2];
    pthread_t thread;
    volatile int running;
#endif
};

#if !defined(__linux__)
static void *timer_worker(void *arg)
{
    struct ast_timer *timer = (struct ast_timer *)arg;
    while (timer->running) {
        if (timer->rate > 0) {
            useconds_t usec = (useconds_t)(1000000 / timer->rate);
            usleep(usec);
            uint64_t tick = 1;
            ssize_t written = write(timer->pipefd[1], &tick, sizeof(tick));
            (void)written;
        } else {
            usleep(20000); // 20ms default (50 Hz)
        }
    }
    return NULL;
}
#endif

struct ast_timer *ast_timer_open(void)
{
    struct ast_timer *timer = (struct ast_timer *)calloc(1, sizeof(struct ast_timer));
    if (!timer) return NULL;

#if defined(__linux__)
    timer->fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timer->fd < 0) {
        free(timer);
        return NULL;
    }
#else
    if (pipe(timer->pipefd) < 0) {
        free(timer);
        return NULL;
    }
    fcntl(timer->pipefd[0], F_SETFL, O_NONBLOCK);
    fcntl(timer->pipefd[1], F_SETFL, O_NONBLOCK);
    timer->fd = timer->pipefd[0];
    timer->running = 1;
    pthread_create(&timer->thread, NULL, timer_worker, timer);
#endif

    timer->rate = 50; // default 50 Hz (20ms)
    return timer;
}

void ast_timer_close(struct ast_timer *timer)
{
    if (!timer) return;

#if defined(__linux__)
    if (timer->fd >= 0) {
        close(timer->fd);
    }
#else
    timer->running = 0;
    if (timer->thread) {
        pthread_join(timer->thread, NULL);
    }
    if (timer->pipefd[0] >= 0) close(timer->pipefd[0]);
    if (timer->pipefd[1] >= 0) close(timer->pipefd[1]);
#endif

    free(timer);
}

int ast_timer_fd(const struct ast_timer *timer)
{
    return timer ? timer->fd : -1;
}

int ast_timer_set_rate(struct ast_timer *timer, unsigned int rate)
{
    if (!timer) return -1;
    timer->rate = (rate > 0) ? (int)rate : 50;

#if defined(__linux__)
    struct itimerspec its;
    long nsec = 1000000000L / timer->rate;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = nsec;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = nsec;
    return timerfd_settime(timer->fd, 0, &its, NULL);
#else
    return 0;
#endif
}

void ast_timer_ack(struct ast_timer *timer, unsigned int quantity)
{
    if (!timer || timer->fd < 0) return;

    uint64_t count = 0;
    ssize_t res = read(timer->fd, &count, sizeof(count));
    (void)res;
}

int ast_timer_enable_continuous(struct ast_timer *timer)
{
    return 0;
}

int ast_timer_disable_continuous(struct ast_timer *timer)
{
    return 0;
}
