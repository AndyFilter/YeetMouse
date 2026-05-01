#ifndef _ACCEL_H
#define _ACCEL_H
#include <linux/module.h>

#include "accel_modes.h"
#include "FixedMath/Fixed64.h"

int accelerate(int *x, int *y);

struct accel_params {
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
    unsigned long lut_size;
    char lut_data[MAX_LUT_BUF_LEN];
    char cc_data_aggregate[MAX_LUT_BUF_LEN];
    FP_LONG rotation_angle;
    FP_LONG angle_snap_threshold;
    FP_LONG angle_snap_angle;
};

#endif /* _ACCEL_H */
