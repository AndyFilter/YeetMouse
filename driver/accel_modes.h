#ifndef ACCEL_MODES_H
#define ACCEL_MODES_H

#include "config.h" // Used in the testing suite
#include <linux/module.h>
#include "FixedMath/Fixed64.h"

#define MAX_LUT_ARRAY_SIZE 128
#define MAX_LUT_BUF_LEN 4096

struct ModesConstants {
    bool is_init;

    // Jump
    FP_LONG C0; // the "integral" evaluated at 0
    FP_LONG r; // basically a smoothness factor

    FP_LONG accel_sub_1;
    FP_LONG exp_sub_1;

    // Power
    FP_LONG offset_x;
    FP_LONG power_constant;

    // Rotation
    FP_LONG sin_a, cos_a;

    // Angle Snapping
    FP_LONG as_sin, as_cos;
    FP_LONG as_half_threshold;
};

extern FP_LONG g_Acceleration, g_Exponent, g_Midpoint, g_RotationAngle, g_AngleSnap_Angle, g_AngleSnap_Threshold, g_LutData_x[], g_LutData_y[];
extern char g_AccelerationMode, g_UseSmoothing;
extern unsigned long g_LutSize;
extern struct ModesConstants modesConst;
static const FP_LONG FP64_PI =   C0NST_FP64_FromDouble(3.14159);
static const FP_LONG FP64_PI_2 = C0NST_FP64_FromDouble(1.57079);
static const FP_LONG FP64_PI_4 = C0NST_FP64_FromDouble(0.78539);
static const FP_LONG FP64_0_1     = 429496736;
static const FP_LONG FP64_1       = 1ll << FP64_Shift;
static const FP_LONG FP64_10      = 10ll << FP64_Shift;
static const FP_LONG FP64_100     = 100ll << FP64_Shift;
static const FP_LONG FP64_1000    = 1000ll << FP64_Shift;
static const FP_LONG FP64_10000   = 10000ll << FP64_Shift;

void update_constants(void);

FP_LONG accel_linear(FP_LONG speed);
FP_LONG accel_power(FP_LONG speed);
FP_LONG accel_classic(FP_LONG speed);
FP_LONG accel_motivity(FP_LONG speed);
FP_LONG accel_natural(FP_LONG speed);
FP_LONG accel_jump(FP_LONG speed);
FP_LONG accel_lut(FP_LONG speed);

#endif //ACCEL_MODES_H
