// SPDX-License-Identifier: GPL-2.0-or-later

#include "accel.h"
#include "util.h"
#include "config.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/string.h>   //strlen
#include "FixedMath/Fixed64.h"
#include "../shared_definitions.h"
#include "accel_modes.h"

MODULE_AUTHOR("Christopher Williams <chilliams (at) gmail (dot) com>"); //Original idea of this module
MODULE_AUTHOR("Klaus Zipfel <klaus (at) zipfel (dot) family>");         //Current maintainer
MODULE_AUTHOR("Maciej Grzęda <gmaciejg525 (at) gmail (dot) com>");      // Current maintainer
// Sorry if you have issues with compilation because of this silly character in my family name lol <3

//Converts a preprocessor define's value in "config.h" to a string - Suspect this to change in future version without a "config.h"
#define _s(x) #x
#define s(x) _s(x)

//Convenient helper for float based parameters, which are passed via a string to this module (must be individually parsed via atof() - available in util.c)
#define PARAM_F(param, default, desc)                           \
    FP_LONG g_##param = C0NST_FP64_FromDouble(default);                     \
    char* g_param_##param = s(default);                  \
    module_param_named(param, g_param_##param, charp, 0644);    \
    MODULE_PARM_DESC(param, desc);

#define PARAM(param, default, desc)                             \
    char g_##param = default;                            \
    module_param_named(param, g_##param, byte, 0644);           \
    MODULE_PARM_DESC(param, desc);

#define PARAM_ARR(param, default, desc) \
    char g_param_##param[MAX_LUT_BUF_LEN] = s(default);                            \
    module_param_string(param, g_param_##param, MAX_LUT_BUF_LEN, 0644);           \
    MODULE_PARM_DESC(param, desc);

#define PARAM_UL(param, default, desc)                           \
    unsigned long g_##param = (unsigned long)default;                     \
    char* g_param_##param = s(default);                            \
    module_param_named(param, g_##param, ulong, 0644);           \
    MODULE_PARM_DESC(param, desc);

// ########## Kernel module parameters

// Simple module parameters (instant update)
//PARAM(no_bind,          0,                  "This will disable binding to this driver via 'yeetmouse_bind' by udev.");
PARAM(update,           1,                  "Triggers an update of the acceleration parameters below");
PARAM(AccelerationMode, ACCELERATION_MODE,  "Sets the algorithm to be used for acceleration");

// Acceleration parameters (type pchar. Converted to float via "update_params" triggered by /sys/module/yeetmouse/parameters/update)
PARAM_F(InputCap,       INPUT_CAP,          "Limit the maximum pointer speed before applying acceleration.");
PARAM_F(Sensitivity,    SENSITIVITY,        "Mouse base sensitivity, or X axis sensitivity if the anisotropy is on."); // Sensitivity for X axis only if sens != sens_y (anisotropy is on), otherwise sensitivity for both axes
PARAM_F(SensitivityY,   SENSITIVITY_Y,      "Mouse base sensitivity on the Y axis."); // Used only when anisotropy is on
PARAM_F(Acceleration,   ACCELERATION,       "Mouse acceleration sensitivity.");
PARAM_F(OutputCap,      OUTPUT_CAP,         "Cap maximum sensitivity.");
PARAM_F(Offset,         OFFSET,             "Mouse acceleration shift.");

PARAM_F(Exponent,       EXPONENT,           "Exponent for algorithms that use it");
PARAM_F(Midpoint,       MIDPOINT,           "Midpoint for sigmoid function, Output Offset for Power mode");
PARAM_F(PreScale,       PRESCALE,           "Parameter to adjust for the DPI");
PARAM  (UseSmoothing,   USE_SMOOTHING,      "Whether to smooth out functions (doesn't apply to all)");
//PARAM_F(ScrollsPerTick, SCROLLS_PER_TICK,   "Amount of lines to scroll per scroll-wheel tick.");

PARAM_UL(LutSize,       LUT_SIZE,           "LUT data array size");
//PARAM_F(LutStride,      LUT_STRIDE,       "Distance between y values for the LUT");
PARAM_ARR(LutDataBuf,   LUT_DATA,           "Data of the LUT stored in a human form"); // g_LutDataBuf should not be used!

PARAM_F(RotationAngle, ROTATION_ANGLE,      "Amount of clockwise rotation (in radians)");
PARAM_F(AngleSnap_Threshold, ANGLE_SNAPPING_THRESHOLD,      "Rotation value at which angle snapping is triggered (in radians)");
PARAM_F(AngleSnap_Angle, ANGLE_SNAPPING_ANGLE,      "Amount of clockwise rotation for angle snapping (in radians)");

FP_LONG g_LutData_x[MAX_LUT_ARRAY_SIZE]; // Array to store the x-values of the LUT data
FP_LONG g_LutData_y[MAX_LUT_ARRAY_SIZE]; // Array to store the y-values of the LUT data

#define FP64_ONE 4294967296ll
#define EXP_ARG_THRESHOLD 16ll

// Converts given string to a unsigned long
unsigned long atoul(const char *str) {
    unsigned long result = 0;
    int i = 0;

    // Iterate through the string, converting each digit to an integer
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }

    return result;
}

// Updates the acceleration parameters. This is purposely done with a delay!
// First, to not hammer too much the logic in "accelerate()", which is called VERY OFTEN!
// Second, to fight possible cheating. However, this can be OFC changed, since we are OSS...
#define PARAM_UPDATE(param) (FP64_FromString(g_param_##param, &g_##param))
#define PARAM_UPDATE_UL(param) (atoul(g_param_##param))

// Aggregate values that don't change with speed to save on calculations done every irq
struct ModesConstants modesConst = { .is_init = false, .C0 = 0, .r = 0, .accel_sub_1 = 0, .exp_sub_1 = 0, .sin_a = 0, .cos_a = 0, .as_cos = 0, .as_sin = 0, .as_half_threshold = 0 };

static ktime_t g_next_update = 0;
INLINE void update_params(ktime_t now)
{
    if(!g_update) return;
    if(now < g_next_update) return;
    g_update = 0;
    g_next_update = now + 1000000000ll;    //Next update is allowed after 1s of delay

    modesConst.is_init = false;

    PARAM_UPDATE(InputCap);
    PARAM_UPDATE(Sensitivity);
    PARAM_UPDATE(SensitivityY);
    PARAM_UPDATE(Acceleration);
    PARAM_UPDATE(OutputCap);
    PARAM_UPDATE(Offset);
    //PARAM_UPDATE(ScrollsPerTick);
    PARAM_UPDATE(Exponent);
    PARAM_UPDATE(Midpoint);
    PARAM_UPDATE(PreScale);
    PARAM_UPDATE(RotationAngle);
    PARAM_UPDATE(AngleSnap_Threshold);
    PARAM_UPDATE(AngleSnap_Angle);
    PARAM_UPDATE_UL(LutSize);
    //PARAM_UPDATE(LutStride);
    if(g_LutSize > MAX_LUT_ARRAY_SIZE)
        g_LutSize = MAX_LUT_ARRAY_SIZE;
    // LutDataBuf get auto updated, we don't need to do anything, just extract the data
    // Populate the g_LutData with the data in the buffer
    char* p = g_param_LutDataBuf;
    int i = 0;
    for(; i < g_LutSize*2 && *p; i++) {
        FP_LONG val;
        p += FP64_FromString(p, &val) + 1; // + 1 to skip the ';'
        // The format for the driver side is very strict tho, so don't edit it by hand pls.
        ((i % 2 == 0) ? g_LutData_x : g_LutData_y)[i/2] = val;

        // Debug stuff (you know it didn't work the first time (nor the 10th time... (that's at least 10 'blue screens')))
        //char buf[25];
        //FP64_ToString(val, buf, 4);
        //printk("YeetMouse: Converted %s, next char is: %i\n", buf, *p);
    }

    // Did not work correctly
    if(i % 2 == 1)
        g_LutSize = 0;

    // Sanity check
    if((g_LutSize <= 1 /*|| g_LutStride == 0*/) && g_AccelerationMode == AccelMode_Lut)
        g_AccelerationMode = AccelMode_Current;

    if (g_AccelerationMode == AccelMode_Lut &&
        (g_LutData_x[g_LutSize-1] == g_LutData_x[g_LutSize-2] && g_LutData_y[g_LutSize-1] == g_LutData_y[g_LutSize-2]))
        g_AccelerationMode = AccelMode_Current;

    // Angle snap threshold should be in range [0, PI)
    if(g_AngleSnap_Threshold >= FP64_PI || g_AngleSnap_Threshold < 0) {
        g_AngleSnap_Threshold = 0;
    }

    update_constants();
}

// Acceleration happens here
int accelerate(int *x, int *y, int *wheel)
{
    FP_LONG delta_x, delta_y, ms, speed;
    //static long buffer_x = 0;
    //static long buffer_y = 0;
    //Static float assignment should happen at compile-time and thus should be safe here. However, avoid non-static assignment of floats outside kernel_fpu_begin()/kernel_fpu_end()
    static FP_LONG carry_x = 0;
    static FP_LONG carry_y = 0;
    //static FP_LONG carry_whl = 0;
    static FP_LONG last_ms = One;
    static ktime_t last;
    ktime_t now;
    int status = 0;

    if(!modesConst.is_init)
        update_constants();

    delta_x = FP64_FromInt(*x);
    delta_y = FP64_FromInt(*y);
    //delta_whl = FP64_FromInt(*wheel);

    //Add buffer values, if present, and reset buffer
    //delta_x = FP64_Add(delta_x, FP64_FromInt((int) buffer_x)); buffer_x = 0;
    //delta_y = FP64_Add(delta_y, FP64_FromInt((int) buffer_y)); buffer_y = 0;

    //Calculate frametime
    now = ktime_get(); // ns
    long long dt = (now - last);
    //int frac = dt % 10000;
    // We can't just store milliseconds as this would lose a lot of precision (nano -> mili, that's 10^-6 difference).
    // But we have only Q16.16 bits of precision, meaning 16 bits for the fractional part of the number (it's constant!).
    // So it would be wasteful to store a millisecond in a fixed point format, because the integral part would be at max like 100
    // and we would lose all the precision on the fractional part, so we move everything storing millis * 100.
    // Now we have at max 10000 to store in the integral part (technically 0xFFFF) and a bit less information in the fractional part
    // that would be lost either way.
    /// THE ABOVE NO LONGER HOLDS, AS I'VE MOVED (AGAIN), THIS TIME TO 64bit FIXED POINT MATH
    //ms = FP64_FromInt(dt / 10000ll) + FP64_Div(FP64_FromInt(frac), fp64_10000); // NOT MILLISECONDS, its ms * 100
    ms = FP64_DivPrecise(FP64_FromInt(dt), FP64_FromInt(1000000));
    last = now;
    //if(ms < 1) ms = last_ms;    //Sometimes, urbs appear bunched -> Beyond µs resolution so the timing reading is plain wrong. Fallback to last known valid frametime
    // Editor node: I have no idea, what this line above really does, but commenting it out solves all my problems
    // with incorrect data. It seems that it tries to fix a problem that doesn't exist, or doesn't exist on my
    // specific setup (PC / System / Mice)
    if(ms > FP64_100) ms = FP64_100;

    //if(ms > 100) ms = 100;      //Original InterAccel has 200 here. RawAccel rounds to 100. So do we.
    last_ms = ms;

    //Update acceleration parameters periodically
    update_params(now);

    //Calculate velocity (one step before rate, which divides rate by the last frametime)
    speed = FP64_Sqrt(FP64_Add(FP64_Mul(delta_x, delta_x), FP64_Mul(delta_y, delta_y)));

    // Apply Pre-Scale
    if(g_PreScale != FP64_1)
        speed = FP64_Mul(speed, g_PreScale);

    //Apply speedcap
    if(g_InputCap > 0){
        //if(speed >= g_InputCap) {
        if(FP64_Sub(speed, g_InputCap) > 0) {
            speed = g_InputCap;
        }
    }

    //Calculate rate from traveled overall distance and add possible rate offsets
    speed = FP64_DivPrecise(speed, ms);
    speed = FP64_Sub(speed, g_Offset);

    // Apply acceleration if movement is over offset
    if (speed > 0) {
        switch (g_AccelerationMode) {
            case AccelMode_Linear:
                speed = accel_linear(speed);
                break;
            case AccelMode_Power:
                speed = accel_power(speed);
                break;
            case AccelMode_Classic:
                speed = accel_classic(speed);
                break;
            case AccelMode_Motivity:
                speed = accel_motivity(speed);
                break;
            case AccelMode_Natural:
                speed = accel_natural(speed);
                break;
            case AccelMode_Jump:
                speed = accel_jump(speed);
                break;
            case AccelMode_Lut: case AccelMode_CustomCurve:
                speed = accel_lut(speed);
                break;
            default:
                speed = FP64_1;
                break;
        }
    } else {
        speed = FP64_1;
    }

    // Actually apply accelerated sensitivity, allow post-scaling and apply carry from previous round
    // Like RawAccel, sensitivity will be a final multiplier:
    if (g_Sensitivity == g_SensitivityY) {
        if(g_Sensitivity != FP64_1)
            speed = FP64_Mul(speed, g_Sensitivity);

        // Apply Output Limit
        if(g_OutputCap > 0)
            speed = FP64_Min(g_OutputCap, speed);

        // Apply acceleration
        delta_x = FP64_Mul(delta_x, speed);
        delta_y = FP64_Mul(delta_y, speed);
    } else {
        speed = FP64_Mul(speed, g_Sensitivity);
        FP_LONG speed_Y = FP64_Mul(speed, g_SensitivityY);

        // Apply Output Limit
        if(g_OutputCap > 0) {
            speed = FP64_Min(g_OutputCap, speed);
            speed_Y = FP64_Min(g_OutputCap, speed_Y);
        }

        // Apply acceleration
        delta_x = FP64_Mul(delta_x, speed);
        delta_y = FP64_Mul(delta_y, speed_Y);
    }

    // Angle Snapping
    if(modesConst.as_half_threshold != 0) {
        FP_LONG delta_mag = FP64_Sqrt(FP64_Add(FP64_Mul(delta_x, delta_x), FP64_Mul(delta_y, delta_y)));
        if (delta_mag != 0) {
            FP_LONG current_angle = FP64_Atan2(delta_y, delta_x);
            FP_LONG angle_diff = FP64_Sub(g_AngleSnap_Angle, current_angle);
            FP_LONG angle_diff_quarter = FP64_PI_2 - FP64_Abs(angle_diff);

            int sign = FP64_Sign(angle_diff_quarter);
            angle_diff_quarter = FP64_Abs(angle_diff_quarter) - FP64_PI_2;

            if (FP64_Abs(angle_diff_quarter) <= modesConst.as_half_threshold) {
                delta_x = FP64_Mul(modesConst.as_cos, delta_mag) * sign;
                delta_y = FP64_Mul(modesConst.as_sin, delta_mag) * sign;
            }
        }
    }

    delta_x = FP64_Add(delta_x, carry_x);
    delta_y = FP64_Add(delta_y, carry_y);

    // I don't do wheel, sorry
    //delta_whl *= g_ScrollsPerTick/3.0f;

    // Apply Rotation after everything else to keep the precision
    if(g_RotationAngle != 0) {
        FP_LONG new_delta_x = FP64_Mul(delta_x, modesConst.cos_a) - FP64_Mul(delta_y, modesConst.sin_a);
        delta_y = FP64_Mul(delta_x, modesConst.sin_a) + FP64_Mul(delta_y, modesConst.cos_a);
        delta_x = new_delta_x;
    }

    //Cast back to int
    *x = FP64_RoundToInt(delta_x);
    *y = FP64_RoundToInt(delta_y);

    //Save carry for next round
    carry_x = FP64_Sub(delta_x, FP64_FromInt(*x));
    carry_y = FP64_Sub(delta_y, FP64_FromInt(*y));
    //carry_whl = delta_whl - *wheel;

    // Used to very roughly estimate the performance, and 0.1% lows
    // ktime_t iter_time = ktime_sub(ktime_get(), now);
    // static ktime_t elapsed_time, highest_elapsed_time;
    // static int iter = 0;
    // elapsed_time += iter_time;
    // if(iter_time > highest_elapsed_time)
    //     highest_elapsed_time = iter_time;
    // if(++iter == 1000) {
    //     iter = 0;
    //     pr_info("YeetMouse: Sum of 1000 iters: %lldns, low 0.1%%: %lldns\n", ktime_to_ns(elapsed_time), highest_elapsed_time);
    //     elapsed_time = 0;
    //     highest_elapsed_time = 0;
    // }

    return status;
}
