// SPDX-License-Identifier: GPL-2.0-or-later

#include "accel.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/string.h>   //strlen
#include "FixedMath/Fixed64.h"
#include "accel_modes.h"
#include "defaults.h"

MODULE_AUTHOR("Christopher Williams <chilliams (at) gmail (dot) com>"); //Original idea of this module
MODULE_AUTHOR("Klaus Zipfel <klaus (at) zipfel (dot) family>");         //Current maintainer
MODULE_AUTHOR("Maciej Grzęda <gmaciejg525 (at) gmail (dot) com>");      // Current maintainer
// Sorry if you have issues with compilation because of this silly character in my family name lol <3

//Converts a preprocessor define's value in "config.h" to a string - Suspect this to change in future version without a "config.h"
#define _s(x) #x
#define s(x) _s(x)

// Convenient helper for float based parameters
#define PARAM_F(param, default, desc)                                   \
    FP_LONG g_##param = C0NST_FP64_FromDouble(default);                 \
    char* g_param_##param = s(default);                                 \
    module_param_named(param, g_param_##param, charp, 0660);            \
    MODULE_PARM_DESC(param, desc);

#define PARAM(param, default, desc)                                     \
    char g_##param = default;                                           \
    module_param_named(param, g_##param, byte, 0660);                   \
    MODULE_PARM_DESC(param, desc);

#define PARAM_BYTE(param, default, desc)                                \
    char g_##param = (char)default;                                     \
    char* g_param_##param = s(default);                                 \
    module_param_named(param, g_param_##param, charp, 0660);            \
    MODULE_PARM_DESC(param, desc);

#define PARAM_ARR(param, default, desc) \
    char g_param_##param[MAX_LUT_BUF_LEN] = s(default);                 \
    module_param_string(param, g_param_##param, MAX_LUT_BUF_LEN, 0660); \
    MODULE_PARM_DESC(param, desc);

#define PARAM_UL(param, default, desc)                                  \
    unsigned long g_##param = (unsigned long)default;                   \
    char* g_param_##param = s(default);                                 \
    module_param_named(param, g_param_##param, charp, 0660);            \
    MODULE_PARM_DESC(param, desc);

// ########## Kernel module parameters

// Simple module parameters (instant update)
PARAM(update,             1,                  "Triggers an update of the acceleration parameters below");

// Triggered update (same as Acceleration parameters)
PARAM_BYTE(AccelerationMode, ACCELERATION_MODE, "Sets the algorithm to be used for acceleration");

// Acceleration parameters (type pchar. Converted to float via "update_params" triggered by /sys/module/yeetmouse/parameters/update)
PARAM_F(InputCap,       INPUT_CAP,          "Limit the maximum pointer speed before applying acceleration.");
PARAM_F(Sensitivity,    SENSITIVITY,        "Mouse base sensitivity, or X axis sensitivity if the anisotropy is on."); // Sensitivity for X axis only if sens != sens_y (anisotropy is on), otherwise sensitivity for both axes
PARAM_F(RatioYX,        RATIO_YX,           "Mouse base sensitivity on the Y axis."); // Used only when anisotropy is on
PARAM_F(OutputCap,      OUTPUT_CAP,         "Cap maximum sensitivity.");
PARAM_F(Offset,         OFFSET,             "Mouse acceleration shift.");
PARAM_F(PreScale,       PRESCALE,           "Parameter to adjust for the DPI");

PARAM_F(Acceleration,   ACCELERATION,       "Mouse acceleration sensitivity.");
PARAM_F(Exponent,       EXPONENT,           "Exponent for algorithms that use it");
PARAM_F(Midpoint,       MIDPOINT,           "Midpoint for sigmoid function, Output Offset for Power mode");
PARAM_F(Motivity,       MOTIVITY,           "Expresses how much change will occur for the Motivity (and Synchronous) function");
PARAM  (UseSmoothing,   USE_SMOOTHING,      "Whether to smooth out functions (doesn't apply to all)");
//PARAM_F(ScrollsPerTick, SCROLLS_PER_TICK,   "Amount of lines to scroll per scroll-wheel tick.");

PARAM_UL(LutSize,       LUT_SIZE,           "LUT data array size");
PARAM_ARR(LutDataBuf,   LUT_DATA,           "Data of the LUT stored in a human form"); // g_LutDataBuf should not be used!

PARAM_ARR(_CustomCurveDataAggregate, CC_DATA_AGGREGATE, "Stores the Custom Curve data, SHOULD NOT BE USED ON THE DRIVER SIDE");

PARAM_F(RotationAngle, ROTATION_ANGLE,      "Amount of clockwise rotation (in radians)");
PARAM_F(AngleSnap_Threshold, ANGLE_SNAPPING_THRESHOLD,      "Rotation value at which angle snapping is triggered (in radians)");
PARAM_F(AngleSnap_Angle, ANGLE_SNAPPING_ANGLE,      "Amount of clockwise rotation for angle snapping (in radians)");

FP_LONG g_LutData_x[MAX_LUT_ARRAY_SIZE]; // Array to store the x-values of the LUT data
FP_LONG g_LutData_y[MAX_LUT_ARRAY_SIZE]; // Array to store the y-values of the LUT data

// Converts given string to a unsigned long
unsigned long atoul(const char *str);

// Updates the acceleration parameters. This is purposely done with a delay!
// First, to not hammer too much the logic in "accelerate()", which is called VERY OFTEN!
// Second, to fight possible cheating. However, this can be OFC changed, since we are OSS...
#define PARAM_UPDATE(param) (FP64_FromString(g_param_##param, &g_##param))
#define PARAM_UPDATE_UL(param) (atoul(g_param_##param))

// Aggregate values that don't change with speed to save on calculations done every irq
struct ModesConstants modesConst = {
    .is_init = false, .C0 = 0, .r = 0, .auxiliar_accel = 0, .auxiliar_constant = 0, .accel_sub_1 = 0, .exp_sub_1 = 0,
    .sin_a = 0, .cos_a = 0, .as_cos = 0, .as_sin = 0, .as_half_threshold = 0, .current_func_at_0 = FP64_1
};

// Acceleration happens here
int accelerate(const struct accel_params * params, struct accel_runtime *rt, const struct ModesConstants *constants, int *x, int *y)
{
    FP_LONG delta_x, delta_y, ms, speed;
    //static long buffer_x = 0;
    //static long buffer_y = 0;
    //static FP_LONG carry_whl = 0;
    static FP_LONG last_ms = One;
    ktime_t now;
    int status = 0;

    delta_x = FP64_FromInt(*x);
    delta_y = FP64_FromInt(*y);
    //delta_whl = FP64_FromInt(*wheel);

    //Add buffer values, if present, and reset buffer
    //delta_x = FP64_Add(delta_x, FP64_FromInt((int) buffer_x)); buffer_x = 0;
    //delta_y = FP64_Add(delta_y, FP64_FromInt((int) buffer_y)); buffer_y = 0;

    //Calculate frametime
    now = ktime_get(); // ns
    long long dt = (now - rt->last);
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
    rt->last = now;
    //if(ms < 1) ms = last_ms;    //Sometimes, urbs appear bunched -> Beyond µs resolution so the timing reading is plain wrong. Fallback to last known valid frametime
    // Editor node: I have no idea, what this line above really does, but commenting it out solves all my problems
    // with incorrect data. It seems that it tries to fix a problem that doesn't exist, or doesn't exist on my
    // specific setup (PC / System / Mice)
    if (ms > FP64_100) ms = FP64_100;

    //if(ms > 100) ms = 100;      //Original InterAccel has 200 here. RawAccel rounds to 100. So do we.
    last_ms = ms;

    // Update acceleration parameters periodically
    // update_params(params, now);

    // Apply Pre-Scale
    if (params->prescale != FP64_1) {
        delta_x = FP64_Mul(delta_x, params->prescale);
        delta_y = FP64_Mul(delta_y, params->prescale);
    }

    // Calculate velocity
    speed = FP64_Sqrt(FP64_Add(FP64_Mul(delta_x, delta_x), FP64_Mul(delta_y, delta_y)));
    speed = FP64_DivPrecise(speed, ms);

    // Apply speedcap
    if (params->input_cap > 0) {
        //if(speed >= params->input_cap) {
        if (FP64_Sub(speed, params->input_cap) > 0) {
            speed = params->input_cap;
        }
    }

    speed = FP64_Sub(speed, params->offset);

    // Apply Rotation before everything else to keep the precision
    if(params->rotation_angle != 0) {
        FP_LONG new_delta_x = FP64_Mul(delta_x, constants->cos_a) - FP64_Mul(delta_y, constants->sin_a);
        delta_y = FP64_Mul(delta_x, constants->sin_a) + FP64_Mul(delta_y, constants->cos_a);
        delta_x = new_delta_x;
    }

    static_assert(AccelMode_Count == 10, "Wrong AccelMode count!");
    // Apply acceleration if movement is over offset
    if (speed > 0) {
        switch (params->acceleration_mode) {
            case AccelMode_Linear:
                speed = accel_linear(constants, params->acceleration, params->use_smoothing, speed);
                break;
            case AccelMode_Power:
                speed = accel_power(constants, params->midpoint, params->acceleration, params->exponent, params->use_smoothing, speed);
                break;
            case AccelMode_Classic:
                speed = accel_classic(constants, params->acceleration, params->use_smoothing, speed);
                break;
            case AccelMode_Motivity:
                speed = accel_motivity(constants, params->midpoint, speed);
                break;
            case AccelMode_Synchronous:
                speed = accel_synchronous(constants, params->acceleration, params->use_smoothing, speed);
                break;
            case AccelMode_Natural:
                speed = accel_natural(constants, params->midpoint, params->use_smoothing, speed);
                break;
            case AccelMode_Jump:
                speed = accel_jump(constants, params->midpoint, params->use_smoothing, speed);
                break;
            case AccelMode_Lut: case AccelMode_CustomCurve:
                speed = accel_lut(params->lut_pairs, params->lut_data_x, params->lut_data_y, speed);
                break;
            default:
                speed = FP64_1;
                break;
        }
    } else {
        speed = constants->current_func_at_0;
    }

    // Actually apply accelerated sensitivity, allow post-scaling and apply carry from previous round
    // Like RawAccel, sensitivity will be a final multiplier:
    if (params->ratio_yx == FP64_1) {
        if(params->sensitivity != FP64_1)
            speed = FP64_Mul(speed, params->sensitivity);

        // Apply Output Limit
        if(params->output_cap > 0)
            speed = FP64_Min(params->output_cap, speed);

        // Apply acceleration
        delta_x = FP64_Mul(delta_x, speed);
        delta_y = FP64_Mul(delta_y, speed);
    } else {
        speed = FP64_Mul(speed, params->sensitivity);
        FP_LONG speed_Y = FP64_Mul(speed, params->ratio_yx);

        // Apply Output Limit
        if(params->output_cap > 0) {
            speed = FP64_Min(params->output_cap, speed);
            speed_Y = FP64_Min(params->output_cap, speed_Y);
        }

        // Apply acceleration
        delta_x = FP64_Mul(delta_x, speed);
        delta_y = FP64_Mul(delta_y, speed_Y);
    }

    // Angle Snapping
    if(constants->as_half_threshold != 0) {
        FP_LONG delta_mag = FP64_Sqrt(FP64_Add(FP64_Mul(delta_x, delta_x), FP64_Mul(delta_y, delta_y)));
        if (delta_mag != 0) {
            FP_LONG current_angle = FP64_Atan2(delta_y, delta_x);
            FP_LONG angle_diff = FP64_Sub(params->angle_snap_angle, current_angle);
            FP_LONG angle_diff_quarter = FP64_PI_2 - FP64_Abs(angle_diff);

            int sign = FP64_Sign(angle_diff_quarter);
            angle_diff_quarter = FP64_Abs(angle_diff_quarter) - FP64_PI_2;

            if (FP64_Abs(angle_diff_quarter) <= constants->as_half_threshold) {
                delta_x = FP64_Mul(constants->as_cos, delta_mag) * sign;
                delta_y = FP64_Mul(constants->as_sin, delta_mag) * sign;
            }
        }
    }

    delta_x = FP64_Add(delta_x, rt->carry_x);
    delta_y = FP64_Add(delta_y, rt->carry_y);

    // I don't do wheel, sorry
    //delta_whl *= params->scrolls_per_tick/3.0f;

    //Cast back to int
    *x = FP64_RoundToInt(delta_x);
    *y = FP64_RoundToInt(delta_y);

    //Save carry for next round
    rt->carry_x = FP64_Sub(delta_x, FP64_FromInt(*x));
    rt->carry_y = FP64_Sub(delta_y, FP64_FromInt(*y));
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
