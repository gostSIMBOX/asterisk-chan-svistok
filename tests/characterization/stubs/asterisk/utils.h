#ifndef SVISTOK_CHARACTERIZATION_ASTERISK_UTILS_H
#define SVISTOK_CHARACTERIZATION_ASTERISK_UTILS_H

#include <stdlib.h>
#include <limits.h>

#define ast_malloc malloc
#define ast_calloc calloc
#define ast_free free

static inline void ast_slinear_saturated_add(short *left, const short *right)
{
    int sum = (int)*left + (int)*right;
    if (sum > SHRT_MAX) sum = SHRT_MAX;
    if (sum < SHRT_MIN) sum = SHRT_MIN;
    *left = (short)sum;
}

#endif
