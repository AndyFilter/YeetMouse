#pragma once

#ifndef CONFIG
#define CONFIG


#ifdef __cplusplus
#define printk printf
extern "C" {
#endif

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>
#define printk printf
#define ktime_t signed long long
struct callback_head {
    struct callback_head *next;
    void (*func)(struct callback_head *head);
} __attribute__((aligned(sizeof(void *))));
#define rcu_head callback_head

#include <driver/FixedMath/Fixed64.h>
static float FP64_ToFloat(FP_LONG v) {
    return (float) v * (1.0f / 4294967296.0f);
}

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifdef __cplusplus
}
#endif


#endif