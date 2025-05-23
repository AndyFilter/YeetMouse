// SPDX-License-Identifier: GPL-2.0-or-later

#include "accel.h"
#include "util.h"
#include "config.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/string.h>   //strlen
#include "FixedMath/Fixed64.h"

MODULE_AUTHOR("Christopher Williams <chilliams (at) gmail (dot) com>"); //Original idea of this module
MODULE_AUTHOR("Klaus Zipfel <klaus (at) zipfel (dot) family>");         //Current maintainer
MODULE_AUTHOR("Maciej Grzęda <gmaciejg525 (at) gmail (dot) com>");      // Current maintainer
// Sorry if you have issues with compilation because of this silly character in my family name lol <3

#define MAX_LUT_ARRAY_SIZE 128
#define MAX_LUT_BUF_LEN 4096

//Converts a preprocessor define's value in "config.h" to a string - Suspect this to change in future version without a "config.h"
#define _s(x) #x
#define s(x) _s(x)

//Convenient helper for float based parameters, which are passed via a string to this module (must be individually parsed via atof() - available in util.c)
#define PARAM_F(param, default, desc)                           \
    FP_LONG g_##param = C0NST_FP64_FromDouble(default);                     \
    static char* g_param_##param = s(default);                  \
    module_param_named(param, g_param_##param, charp, 0644);    \
    MODULE_PARM_DESC(param, desc);

#define PARAM(param, default, desc)                             \
    static char g_##param = default;                            \
    module_param_named(param, g_##param, byte, 0644);           \
    MODULE_PARM_DESC(param, desc);

#define PARAM_ARR(param, default, desc) \
    static char g_param_##param[MAX_LUT_BUF_LEN] = s(default);                            \
    module_param_string(param, g_param_##param, MAX_LUT_BUF_LEN, 0644);           \
    MODULE_PARM_DESC(param, desc);

#define PARAM_UL(param, default, desc)                           \
    unsigned long g_##param = (unsigned long)default;                     \
    static char* g_param_##param = s(default);                            \
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

const FP_LONG FP64_PI = C0NST_FP64_FromDouble(3.14159);
const FP_LONG FP64_PI_2 = C0NST_FP64_FromDouble(1.57079);
const FP_LONG FP64_PI_4 = C0NST_FP64_FromDouble(0.78539);
const FP_LONG FP64_0_1     = 429496736;
const FP_LONG FP64_1    = 1ll << FP64_Shift;
const FP_LONG FP64_10      = 10ll << FP64_Shift;
const FP_LONG FP64_100     = 100ll << FP64_Shift;
const FP_LONG FP64_1000    = 1000ll << FP64_Shift;
const FP_LONG FP64_10000    = 10000ll << FP64_Shift;

// Aggregate values that don't change with speed to save on calculations done every irq
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
} modesConst = { .is_init = false, .C0 = 0, .r = 0, .accel_sub_1 = 0, .exp_sub_1 = 0, .sin_a = 0, .cos_a = 0, .as_cos = 0, .as_sin = 0, .as_half_threshold = 0 };

// Recalculate new modes constants
static void update_constants(void) {
    // General
    modesConst.accel_sub_1 = FP64_Sub(g_Acceleration, FP64_ONE);
    modesConst.exp_sub_1 = FP64_Sub(g_Exponent, FP64_ONE);

    // Jump
    if (g_AccelerationMode == 5) {
        if (g_Exponent == 0 || g_Midpoint == 0) {
            printk("YeetMouse: Warning: Acceleration mode 'Jump' is not supported for exponent 0 midpoint 0. Falling back to mode 1.\n");
            g_Exponent = 1;
            g_Midpoint = 1;
            g_Acceleration = 0;
            g_AccelerationMode = 1;
        }
        else {
            modesConst.r = FP64_DivPrecise(FP64_Mul(Two, Pi), FP64_Mul(g_Exponent, g_Midpoint));
            FP_LONG r_times_m = FP64_Mul(modesConst.r, g_Midpoint);

            // Safely exponentiate without overflow (ln(1+exp(x)) when x -> 'inf' = ln(exp(x)) = x. (in practice works for x >= 8))
            if (r_times_m < (EXP_ARG_THRESHOLD << FP64_Shift))
                modesConst.C0 = FP64_Mul(FP64_DivPrecise(FP64_Log(FP64_Add(1, FP64_Exp(r_times_m))), modesConst.r), modesConst.accel_sub_1);
            else
                modesConst.C0 = FP64_Mul(modesConst.accel_sub_1, g_Midpoint);
        }
    }

    // Power
    if (g_AccelerationMode == 2) {
        if (g_Exponent == 0 || g_Exponent == -FP64_ONE || g_Acceleration == 0) {
            printk("YeetMouse: Warning: Acceleration mode 'Power' is not supported for exponent 0 or -1 or acceleration 0. Falling back to mode 1.\n");
            g_Acceleration = 0;
            g_AccelerationMode = 1;
        }
        else if (g_Midpoint == 0) {
            modesConst.offset_x = 0;
            modesConst.power_constant = 0;
        }
        else {
            // modesConst.offset_x = FP64_DivPrecise(FP64_Pow(FP64_DivPrecise(g_Midpoint, FP64_Add(g_Exponent, FP64_ONE)),
            //     FP64_DivPrecise(FP64_ONE, g_Exponent)), g_Acceleration);
            // modesConst.power_constant = FP64_DivPrecise(FP64_Mul(modesConst.offset_x, FP64_Mul(g_Midpoint, g_Exponent)), FP64_Add(g_Exponent, FP64_ONE));

            FP_LONG exponent_plus_one = FP64_Add(g_Exponent, FP64_ONE);
            FP_LONG one_over_exponent = FP64_DivPrecise(FP64_ONE, g_Exponent);
            FP_LONG base_value = FP64_DivPrecise(g_Midpoint, exponent_plus_one);

            FP_LONG pow_result = FP64_Pow(base_value, one_over_exponent);
            modesConst.offset_x = FP64_DivPrecise(pow_result, g_Acceleration);

            FP_LONG intermediate = FP64_Mul(modesConst.offset_x, FP64_Mul(g_Midpoint, g_Exponent));
            modesConst.power_constant = FP64_DivPrecise(intermediate, exponent_plus_one);
        }
    }

    // Rotation (precalculate the trig. functions)
    modesConst.sin_a = FP64_Sin(g_RotationAngle);
    modesConst.cos_a = FP64_Cos(g_RotationAngle);

    modesConst.as_cos = FP64_Cos(g_AngleSnap_Angle);
    modesConst.as_sin = FP64_Sin(g_AngleSnap_Angle);
    modesConst.as_half_threshold = FP64_DivPrecise(g_AngleSnap_Threshold, 2ll << FP64_Shift);

    modesConst.is_init = true;
}

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
    if((g_LutSize <= 1 /*|| g_LutStride == 0*/) && g_AccelerationMode == 6)
        g_AccelerationMode = 1;

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
    static ktime_t last;//, elapsed_time, highest_elapsed_time;
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
    if(speed > 0)
    {
        // Linear acceleration
        if(g_AccelerationMode == 1) {
            // Speed * Acceleration
            //speed *= g_Acceleration;
            //speed += 1;

            // FIXED-POINT:
            speed = FP64_Mul(speed, g_Acceleration);
            speed = FP64_Add(FP64_1, speed);
        }

        // Power acceleration
        else if(g_AccelerationMode == 2) {
            // (Speed * Acceleration)^Exponent + constant / x

            if (speed <= modesConst.offset_x)
                speed = g_Midpoint;
            else if (modesConst.power_constant == 0)
                speed = FP64_PowFast(FP64_Mul(speed, g_Acceleration), g_Exponent);
            else
                speed = FP64_Add(FP64_PowFast(FP64_Mul(speed, g_Acceleration), g_Exponent), FP64_DivPrecise(modesConst.power_constant, speed));
        }

        // Classic acceleration
        else if(g_AccelerationMode == 3) {
            // (Speed * Acceleration) ^ (Exponent - 1) + 1
            // Same as above just without adding the one
            //speed *= g_Acceleration;
            //speed += 1;
            //B_pow(&speed, &g_Exponent);

            // FIXED-POINT:
            speed = FP64_Mul(speed, g_Acceleration);
            speed = FP64_PowFast(speed, modesConst.exp_sub_1);
            speed = FP64_Add(speed, FP64_1);
        }

        // Motivity (Sigmoid function)
        else if(g_AccelerationMode == 4) {
            // Acceleration / ( 1 + e ^ (midpoint - x))
            //product = g_Midpoint-speed;
            //motivity = e;
            //B_pow(&motivity, &product);
            //motivity = g_Acceleration / (1 + motivity);
            //speed = motivity;

            // FIXED-POINT:
            FP_LONG exp = FP64_ExpFast(FP64_Sub(g_Midpoint, speed));
            speed = FP64_Add(FP64_1, FP64_DivPrecise(modesConst.accel_sub_1, FP64_Add(FP64_1, exp)));
        }

        // Jump / Smooth Jump
        else if(g_AccelerationMode == 5) {
            // r = 2pi/(k*midpoint), where k is the smoothness factor (stored inside g_Exponent)
            // Jump: Acceleration / (1 + exp(r(midpoint - x))) + 1
            // Smooth: Integral of the above divided by x pretty much

            FP_LONG exp_arg = FP64_Mul(modesConst.r, FP64_Sub(g_Midpoint, speed));
            FP_LONG D = FP64_ExpFast(exp_arg);

            if(g_UseSmoothing) { // smooth
                FP_LONG natural_log = exp_arg > (EXP_ARG_THRESHOLD << FP64_Shift) ? exp_arg : FP64_LogFast(FP64_Add(FP64_1, D));
                FP_LONG integral = FP64_Mul(modesConst.accel_sub_1, FP64_Add(speed, FP64_DivPrecise(natural_log, modesConst.r)));
                // Not really an integral
                speed = FP64_Add(FP64_DivPrecise(FP64_Sub(integral, modesConst.C0), speed), FP64_1);
            }
            else {
                speed = FP64_Add(FP64_DivPrecise(modesConst.accel_sub_1, FP64_Add(FP64_1, D)), FP64_1);
            }
        }

        // LUT (Look-Up-Table) / Custom Curve
        else if(g_AccelerationMode == 6 || g_AccelerationMode == 7) {
            // Assumes the size and values are valid. Please don't change LUT parameters by hand.

            if(speed < g_LutData_x[0]) // Check if the speed is below the first given point
                speed = g_LutData_y[0];
            else {
                int l = 0, r = g_LutSize - 1, best_point = r, iter = 0; // We REALLY don't want an infinity loop in kernel
                while (l <= r && iter < 10) {
                    int mid = (r + l) / 2;

                    if (speed > g_LutData_x[mid]) {
                        l = mid + 1;
                    } else {
                        best_point = mid;
                        r = mid - 1;
                    }

                    iter++;
                }

                int index = best_point-1;

                FP_LONG p = g_LutData_y[index];
                FP_LONG p1 = g_LutData_y[index + 1];

                // denominator should not possibly ever be equal to 0 here... (we all know how this will end)
                FP_LONG frac = FP64_DivPrecise(speed - g_LutData_x[index],
                                               g_LutData_x[index + 1] - g_LutData_x[index]);

                speed = FP64_Lerp(p, p1, frac);
            }
        }

        else {
            speed = FP64_1;
        }
    }
    else
        speed = FP64_1; // Set speed to 1 if "below offset"

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
    //ktime_t iter_time = ktime_sub(ktime_get(), now);
    //elapsed_time += iter_time;
    //if(iter_time > highest_elapsed_time)
    //    highest_elapsed_time = iter_time;
    //if(++iter == 1000) {
    //    iter = 0;
    //    pr_info("YeetMouse: Sum of 1000 iters: %lldns, low 0.1%%: %lldns\n", ktime_to_ns(elapsed_time), highest_elapsed_time);
    //    elapsed_time = 0;
    //    highest_elapsed_time = 0;
    //}

    return status;
}