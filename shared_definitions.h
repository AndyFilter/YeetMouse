#pragma once

#ifndef SHARED_DEFINITIONS_H
#define SHARED_DEFINITIONS_H
#include <linux/types.h>
#include "driver/FixedMath/Fixed64.h"

#define SYNC_START (-3)
#define SYNC_STOP (9)
#define SYNC_NUM (8)
#define SYNC_CAPACITY ((SYNC_STOP - SYNC_START) * SYNC_NUM + 1)

enum AccelMode {
    AccelMode_Current = 0, // Mainly used in GUI, denotes lack of a curve on the driver side
    AccelMode_Linear = 1,
    AccelMode_Power = 2,
    AccelMode_Classic = 3,
    AccelMode_Motivity = 4,
    AccelMode_Synchronous = 5,
    AccelMode_Natural = 6,
    AccelMode_Jump = 7,
    AccelMode_Lut = 8,
    AccelMode_CustomCurve = 9,
    AccelMode_Count,
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

#endif
