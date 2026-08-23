#ifndef _ACCEL_H
#define _ACCEL_H

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

struct ModesConstants {
    bool is_init;

    // General
    FP_LONG accel_sub_1;
    FP_LONG exp_sub_1;
    FP_LONG current_func_at_0;

    // Synchronous (legacy)
    FP_LONG logMot;
    FP_LONG gammaConst;
    FP_LONG logSync;
    FP_LONG sharpness;
    FP_LONG sharpnessRecip;
    bool useClamp;
    FP_LONG minSens;
    FP_LONG maxSens;

    // Classic
    FP_LONG sign;
    FP_LONG gain_constant;
    FP_LONG cap_x;
    FP_LONG cap_y;

    // Jump
    FP_LONG C0; // the "integral" evaluated at 0
    FP_LONG r; // basically a smoothness factor

    // Power
    FP_LONG offset_x;
    FP_LONG power_constant;

    // Natural
    FP_LONG auxiliar_accel;
    FP_LONG auxiliar_constant;

    // Rotation
    FP_LONG sin_a, cos_a;

    // Angle Snapping
    FP_LONG as_sin, as_cos;
    FP_LONG as_half_threshold;

    bool lut_ready;                  // whether the synchronous smoothing LUT below is built
    FP_LONG x_start;                 // 2^SYNC_START
    FP_LONG data[SYNC_CAPACITY];     // monotonic over x
};

int accelerate(const struct accel_params * params, struct accel_runtime *rt, const struct ModesConstants *constants, int *x, int *y);

#endif /* _ACCEL_H */
