#ifndef ACCEL_MODES_H
#define ACCEL_MODES_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TEST_ENV
#include "tests/config.h"
#endif
#include <linux/module.h>
#include "FixedMath/Fixed64.h"
#include "accel.h"
#include "../shared_definitions.h"

extern unsigned long g_LutSize;
static const FP_LONG FP64_PI =   C0NST_FP64_FromDouble(3.14159);
static const FP_LONG FP64_PI_2 = C0NST_FP64_FromDouble(1.57079);
static const FP_LONG FP64_PI_4 = C0NST_FP64_FromDouble(0.78539);
static const FP_LONG FP64_0_1     = 429496736ll;
static const FP_LONG FP64_0_01    = C0NST_FP64_FromDouble(0.001);
static const FP_LONG FP64_0_5     = 2147483648ll;
static const FP_LONG FP64_1       = 1ll << FP64_Shift;
static const FP_LONG FP64_10      = 10ll << FP64_Shift;
static const FP_LONG FP64_100     = 100ll << FP64_Shift;
static const FP_LONG FP64_1000    = 1000ll << FP64_Shift;
static const FP_LONG FP64_10000   = 10000ll << FP64_Shift;

void update_constants(struct accel_params *params, struct ModesConstants *constants);

FP_LONG accel_linear(const struct ModesConstants *constants, FP_LONG acceleration, bool use_smoothing, FP_LONG speed);
FP_LONG accel_power(const struct ModesConstants *constants, FP_LONG midpoint, FP_LONG acceleration, FP_LONG exponent, bool use_smoothing, FP_LONG speed);
FP_LONG accel_classic(const struct ModesConstants *constants, FP_LONG acceleration, bool use_smoothing, FP_LONG speed);
FP_LONG accel_motivity(const struct ModesConstants *constants, FP_LONG midpoint, FP_LONG speed);
FP_LONG accel_synchronous(const struct ModesConstants *constants, FP_LONG acceleration, bool use_smoothing, FP_LONG speed);
FP_LONG accel_natural(const struct ModesConstants *constants, FP_LONG midpoint, bool use_smoothing, FP_LONG speed);
FP_LONG accel_jump(const struct ModesConstants *constants, FP_LONG midpoint, bool use_smoothing, FP_LONG speed);
FP_LONG accel_lut(unsigned long lut_pairs, const FP_LONG lut_data_x[MAX_LUT_ARRAY_SIZE], const FP_LONG lut_data_y[MAX_LUT_ARRAY_SIZE], FP_LONG speed);

#ifdef __cplusplus
}
#endif

#endif //ACCEL_MODES_H
