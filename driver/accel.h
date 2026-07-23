#ifndef _ACCEL_H
#define _ACCEL_H
#include <linux/module.h>
#include <linux/types.h>

#include "FixedMath/Fixed64.h"
#include "../shared_definitions.h"

#define MAX_LUT_ARRAY_SIZE 128
#define MAX_LUT_BUF_LEN 4096

struct accel_runtime {
    FP_LONG   carry_x, carry_y;
    FP_LONG   last_ms;
    ktime_t   last;
};

struct accel_params {
    struct rcu_head rcu;
    char acceleration_mode;
    FP_LONG input_cap;
    FP_LONG sensitivity;
    FP_LONG ratio_yx;
    FP_LONG output_cap;
    FP_LONG offset;
    FP_LONG prescale;
    FP_LONG acceleration;
    FP_LONG exponent;
    FP_LONG midpoint;
    FP_LONG motivity;
    bool use_smoothing;
    unsigned long lut_pairs;
    FP_LONG lut_data_x[MAX_LUT_ARRAY_SIZE];
    FP_LONG lut_data_y[MAX_LUT_ARRAY_SIZE];
    char cc_data_aggregate[MAX_LUT_BUF_LEN];
    FP_LONG rotation_angle;
    FP_LONG angle_snap_threshold;
    FP_LONG angle_snap_angle;
};

int accelerate(const struct accel_params * params, struct accel_runtime *rt, const struct ModesConstants *constants, int *x, int *y);

#endif /* _ACCEL_H */
