#pragma once

#ifndef CONFIG
#define CONFIG

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>
#define printk printf

#include "FixedMath/Fixed64.h"
static float FP64_ToFloat(FP_LONG v) {
    return (float) v * (1.0f / 4294967296.0f);
}

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif



#endif