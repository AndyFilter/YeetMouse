#include "accel_modes.h"

#include "../shared_definitions.h"
#include "FixedMath/Fixed64.h"
#include "FixedMath/FixedUtil.h"

#define EXP_ARG_THRESHOLD 16ll

// Recalculate new modes constants
void update_constants(void) {
    // General
    modesConst.accel_sub_1 = FP64_Sub(g_Acceleration, FP64_1);
    modesConst.exp_sub_1 = FP64_Sub(g_Exponent, FP64_1);
    modesConst.classic_cap_x = 0;
    modesConst.classic_cap_y = 0;
    modesConst.classic_constant = 0;
    modesConst.classic_sign = FP64_1;

    // Synchronous
    if (g_AccelerationMode == AccelMode_Synchronous) {
        if (g_Motivity <= FP64_1) {
            printk("YeetMouse: Error: Acceleration mode 'Synchronous' is not supported for motivity 1.\n");
            g_Acceleration = 0;
            g_AccelerationMode = AccelMode_Current;
        }
        else {
            modesConst.logMot = FP64_Log(g_Motivity);
            modesConst.gammaConst = FP64_DivPrecise(g_Exponent, modesConst.logMot);
            modesConst.logSync = FP64_Log(g_Acceleration);

            // sharpness = (midpoint == 0) ? 16.0 : (0.5 / midpoint)
            modesConst.sharpness = (g_Midpoint == 0)
                ? FP64_FromInt(16)
                : FP64_DivPrecise(FP64_0_5, g_Midpoint);

            modesConst.sharpnessRecip = FP64_DivPrecise(FP64_1, modesConst.sharpness);
            modesConst.useClamp = (modesConst.sharpness >= FP64_FromInt(16));

            modesConst.minSens = FP64_DivPrecise(FP64_1, g_Motivity);
            modesConst.maxSens = g_Motivity;
        }
    }

    // Classic
    if (g_AccelerationMode == AccelMode_Classic) {
            if (g_UseSmoothing && (g_Exponent == 0 || modesConst.exp_sub_1 == 0)) {
            printk("YeetMouse: Error: Acceleration mode 'Classic' is not supported for exponent 0 or 1 while using the the smooth cap.\n");
            g_Exponent = FP64_FromFloat(2.1f);
            g_Midpoint = FP64_FromFloat(6.f);
            g_Acceleration = FP64_FromFloat(0.15f);
            g_UseSmoothing = false;
            g_AccelerationMode = AccelMode_Current;
        } else {
            if (g_UseSmoothing) {
                FP_LONG sign = FP64_1;
                FP_LONG cap_y = FP64_Sub(g_Midpoint, FP64_1);
                FP_LONG cap_x = FP64_FromInt(0);
                FP_LONG constant = FP64_FromInt(0);
                if (cap_y != 0) {
                    if (cap_y < 0) {
                    cap_y = FP64_Mul(cap_y, Neg1);
                    sign = Neg1;
                    }
                    cap_x = FP64_DivPrecise(FP64_Pow(FP64_DivPrecise(cap_y, g_Exponent), FP64_DivPrecise( FP64_1, modesConst.exp_sub_1)), g_Acceleration);
                }
                FP_LONG factor = FP64_DivPrecise(FP64_Sub(g_Exponent, FP64_1), g_Exponent);
                constant = FP64_Mul(cap_y, cap_x);
                constant = FP64_Mul(factor, constant);
                constant = FP64_Mul(constant, Neg1);
                modesConst.classic_cap_x = cap_x;
                modesConst.classic_cap_y = cap_y;
                modesConst.classic_constant = constant;
                modesConst.classic_sign = sign;
            }
        }
    }

    // Natural
    if (g_AccelerationMode == AccelMode_Natural) {
        if (modesConst.exp_sub_1 == 0 || g_Exponent == FP64_1) {
            printk("YeetMouse: Error: Acceleration mode 'Natural' is not supported for exponent 1.\n");
            g_Acceleration = 0;
            g_AccelerationMode = AccelMode_Current;
        }
        if (g_Acceleration == 0) {
            printk("YeetMouse: Error: Acceleration mode 'Natural' is not supported for acceleration 0.\n");
            g_Acceleration = 0;
            g_AccelerationMode = AccelMode_Current;
        }
        else {
            modesConst.auxiliar_accel = FP64_DivPrecise(g_Acceleration, FP64_Abs(modesConst.exp_sub_1));
            modesConst.auxiliar_constant = FP64_DivPrecise(-modesConst.exp_sub_1, modesConst.auxiliar_accel);
        }
    }

    // Jump
    if (g_AccelerationMode == AccelMode_Jump) {
        if (g_Exponent == 0 || g_Midpoint == 0) {
            printk("YeetMouse: Error: Acceleration mode 'Jump' is not supported for exponent 0 or midpoint 0.\n");
            g_Exponent = 1;
            g_Midpoint = 1;
            g_Acceleration = 0;
            g_AccelerationMode = AccelMode_Current;
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
    if (g_AccelerationMode == AccelMode_Power) {
        if (g_Exponent == 0 || g_Exponent == -FP64_1 || g_Acceleration == 0) {
            printk("YeetMouse: Error: Acceleration mode 'Power' is not supported for exponent 0 or -1 or acceleration 0.\n");
            g_Acceleration = 0;
            g_AccelerationMode = AccelMode_Current;
        }
        else if (g_Midpoint == 0) {
            modesConst.offset_x = 0;
            modesConst.power_constant = 0;
        }
        else if (FP64_DivPrecise(g_Midpoint, FP64_Mul(g_Acceleration, g_Exponent)) > FP64_100) { // 100 here is completely arbitrary
            printk("YeetMouse: Error: Invalid parameters for the 'Power' mode.\n");
            g_Acceleration = 0;
            g_AccelerationMode = AccelMode_Current;
        }
        else {
            // modesConst.offset_x = FP64_DivPrecise(FP64_Pow(FP64_DivPrecise(g_Midpoint, FP64_Add(g_Exponent, FP64_ONE)),
            //     FP64_DivPrecise(FP64_ONE, g_Exponent)), g_Acceleration);
            // modesConst.power_constant = FP64_DivPrecise(FP64_Mul(modesConst.offset_x, FP64_Mul(g_Midpoint, g_Exponent)), FP64_Add(g_Exponent, FP64_ONE));

            FP_LONG exponent_plus_one = FP64_Add(g_Exponent, FP64_1);
            FP_LONG one_over_exponent = FP64_DivPrecise(FP64_1, g_Exponent);
            FP_LONG base_value = FP64_DivPrecise(g_Midpoint, exponent_plus_one);

            FP_LONG pow_result = FP64_Pow(base_value, one_over_exponent);
            modesConst.offset_x = FP64_DivPrecise(pow_result, g_Acceleration);

            FP_LONG intermediate = FP64_Mul(modesConst.offset_x, FP64_Mul(g_Midpoint, g_Exponent));
            modesConst.power_constant = FP64_DivPrecise(intermediate, exponent_plus_one);
        }
    }

    // Lut (Validation)
    if (g_AccelerationMode == AccelMode_Lut) {
        if (g_LutSize <= 1|| g_LutData_x[g_LutSize-1] == g_LutData_x[g_LutSize-2])
            g_AccelerationMode = AccelMode_Current;
    }

    // Check if LUT_x is sorted
    for (int i = 1; i < g_LutSize; i++) {
        if (g_LutData_x[i - 1] > g_LutData_x[i]) {
            g_AccelerationMode = AccelMode_Current;
            printk("YeetMouse: Error: Acceleration mode 'LUT' is not supported for unsorted LUT_x.\n");
            break;
        }
    }

    // Rotation (precalculate the trig. functions)
    modesConst.sin_a = FP64_Sin(g_RotationAngle);
    modesConst.cos_a = FP64_Cos(g_RotationAngle);

    modesConst.as_cos = FP64_Cos(g_AngleSnap_Angle);
    modesConst.as_sin = FP64_Sin(g_AngleSnap_Angle);
    modesConst.as_half_threshold = FP64_DivPrecise(g_AngleSnap_Threshold, 2ll << FP64_Shift);

    modesConst.is_init = 1;
}

#define SYNC_START (-3)
#define SYNC_STOP (9)
#define SYNC_NUM (8)
#define SYNC_CAPACITY ((SYNC_STOP - SYNC_START) * SYNC_NUM + 1)

// Local LUT storage for synchronous smoothing
static struct {
    FP_LONG x_start;                 // 2^SYNC_START
    FP_LONG data[SYNC_CAPACITY];     // monotonic over x
} s_sync_lut;

static bool s_sync_lut_ready = false;

static FP_LONG synchronous_legacy(FP_LONG x) {
    if (modesConst.useClamp) {
        FP_LONG L = FP64_Mul(modesConst.gammaConst, FP64_Sub(FP64_Log(x), modesConst.logSync));
        if (L < FP64_1) return modesConst.minSens;
        if (L > -FP64_1) return modesConst.maxSens;
        return FP64_Exp(FP64_Mul(L, modesConst.logMot));
    }

    if (x == g_Acceleration) {
        return FP64_1;
    }

    FP_LONG delta = FP64_Sub(FP64_Log(x), modesConst.logSync);
    FP_LONG M = FP64_Mul(modesConst.gammaConst, FP64_Abs(delta));
    FP_LONG T = FP64_Tanh(FP64_Pow(M, modesConst.sharpness));
    FP_LONG exponent = FP64_Pow(T, modesConst.sharpnessRecip);
    if (delta < 0) {
        exponent = -exponent;
    }
    return FP64_Exp(FP64_Mul(exponent, modesConst.logMot));
}

// Helper: build LUT for smoothing/gain mode
static bool synchronous_build_lut(void) {
    // x_start = 2^SYNC_START
    s_sync_lut.x_start = FP64_Scalbn(FP64_1, SYNC_START);

    FP_LONG sum = 0;
    FP_LONG prev_x   = 0;

    int idx = 0;

    // integrate sync_legacy in small steps using the same 2-point midpoint rule
    for (int e = 0; e < (SYNC_STOP - SYNC_START); ++e) {
        // expScale = 2^(e + SYNC_START) / SYNC_NUM
        FP_LONG expScale = FP64_DivPrecise(FP64_Scalbn(FP64_1, e + SYNC_START), FP64_FromInt(SYNC_NUM));

        for (int i = 0; i < SYNC_NUM; ++i) {
            // b = (i + SYNC_NUM) * expScale   [sweeps from 2^(e+SYNC_START) .. 2^(e+1+SYNC_START)]
            FP_LONG b = FP64_Mul(FP64_FromInt(i + SYNC_NUM), expScale);

            // integrate from a -> b in two equal partitions
            FP_LONG interval = FP64_DivPrecise(FP64_Sub(b, prev_x), FP64_FromInt(2));
            for (int p = 1; p <= 2; ++p) {
                // xi = a + p*interval
                FP_LONG xi = FP64_Add(prev_x, FP64_Mul(FP64_FromInt(p), interval));
                // sum += sync_legacy(xi) * interval
                sum = FP64_Add(sum, FP64_Mul(synchronous_legacy(xi), interval));
            }

            prev_x = b;

            s_sync_lut.data[idx++] = sum;
        }
    }

    // final point at 2^SYNC_STOP
    {
        FP_LONG b = FP64_Scalbn(FP64_1, SYNC_STOP);
        FP_LONG interval = FP64_DivPrecise(FP64_Sub(b, prev_x), FP64_FromInt(2));
        for (int p = 1; p <= 2; ++p) {
            FP_LONG xi = FP64_Add(prev_x, FP64_Mul(FP64_FromInt(p), interval));
            sum = FP64_Add(sum, FP64_Mul(synchronous_legacy(xi), interval));
        }
        prev_x = b;

        if (idx < SYNC_CAPACITY) {
            s_sync_lut.data[idx] = sum; // last element
        }
    }

    s_sync_lut_ready = true;
    return true;
}

static FP_LONG synchronous_eval(FP_LONG x) {
    // Find octave index: e = floor(log2(x)), clamped
    int e = FP64_Ilogb(x);
    if (e < SYNC_START) e = SYNC_START;
    if (e > (SYNC_STOP - 1)) e = SYNC_STOP - 1;

    // frac in [0,1): frac = x / 2^e - 1
    FP_LONG frac = FP64_Sub(FP64_Scalbn(x, -e), FP64_1);

    // idxF = SYNC_NUM * ((e - SYNC_START) + frac)
    FP_LONG idxF = FP64_Mul(
        FP64_FromInt(SYNC_NUM),
        FP64_Add(FP64_FromInt(e - SYNC_START), frac)
    );

    // idx = floor(idxF), clamped to [0, SYNC_CAPACITY-2]
    int idx = FP64_FloorToInt(idxF);
    if (idx > (SYNC_CAPACITY - 2)) idx = SYNC_CAPACITY - 2;

    if (idx >= 0) {
        // t = fractional part in [0,1)
        FP_LONG t = FP64_Sub(idxF, FP64_FromInt(idx));

        FP_LONG y = FP64_Lerp(s_sync_lut.data[idx], s_sync_lut.data[idx + 1], t);

        return FP64_DivPrecise(y, x);
    }
    FP_LONG y = s_sync_lut.data[0];
    return FP64_DivPrecise(y, s_sync_lut.x_start);
}

FP_LONG accel_linear(FP_LONG speed) {
    speed = FP64_Mul(speed, g_Acceleration);
    return FP64_Add(FP64_1, speed);
}

FP_LONG accel_power(FP_LONG speed) {
    if (speed <= modesConst.offset_x)
        speed = g_Midpoint;
    else if (modesConst.power_constant == 0)
        speed = FP64_PowFast(FP64_Mul(speed, g_Acceleration), g_Exponent);
    else
        speed = FP64_Add(FP64_PowFast(FP64_Mul(speed, g_Acceleration), g_Exponent), FP64_DivPrecise(modesConst.power_constant, speed));
    return speed;
}

FP_LONG accel_classic(FP_LONG speed) {
    // (Speed * Acceleration) ^ (Exponent - 1) + 1
    // Same as above just without adding the one
    //speed *= g_Acceleration;
    //speed += 1;
    //B_pow(&speed, &g_Exponent);

    // FIXED-POINT:
    FP_LONG accel_classic_result = speed;
    accel_classic_result = FP64_Mul(accel_classic_result, g_Acceleration);
    accel_classic_result = FP64_PowFast(accel_classic_result, modesConst.exp_sub_1);

    // if Use Smooth Cap is on, we proceed to calculate the transition
    // point and the function that provides the smooth cap
    if (g_UseSmoothing) {
	// we setup the y cap
        if (speed < modesConst.classic_cap_x) {
            accel_classic_result = FP64_Mul(modesConst.classic_sign, accel_classic_result);
            speed = FP64_Add(accel_classic_result, FP64_1);
	} else {
            speed = FP64_Add(FP64_Mul(modesConst.classic_sign, FP64_Add(FP64_DivPrecise(modesConst.classic_constant, speed), modesConst.classic_cap_y)), FP64_1);
	}
    } else {
	speed = FP64_Add(accel_classic_result, FP64_1);
    }
    return speed;
}

FP_LONG accel_motivity(FP_LONG speed) {
    // Acceleration / ( 1 + e ^ (midpoint - x))
    //product = g_Midpoint-speed;
    //motivity = e;
    //B_pow(&motivity, &product);
    //motivity = g_Acceleration / (1 + motivity);
    //speed = motivity;

    // FIXED-POINT:
    FP_LONG exp = FP64_ExpFast(FP64_Sub(g_Midpoint, speed));
    speed = FP64_Add(FP64_1, FP64_DivPrecise(modesConst.accel_sub_1, FP64_Add(FP64_1, exp)));
    return speed;
}

FP_LONG accel_synchronous(FP_LONG speed) {
    // Defensive: ensure speed > 0 for log-domain math; you can clamp differently if your file already does.
    if (speed <= 0) {
        return FP64_1;
    }

    FP_LONG val;
    if (g_UseSmoothing) {
        if (!s_sync_lut_ready) { // This should be skipped 100% (except the first time) of the time by the branch predictor
            synchronous_build_lut();
        }
        val = synchronous_eval(speed);
    } else {
        val = synchronous_legacy(speed);
    }
    return val;
}


FP_LONG accel_jump(FP_LONG speed) {
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
    return speed;
}

FP_LONG accel_natural(FP_LONG speed) {
    if (speed <= g_Midpoint) {
        speed = FP64_1;
    } else {
        FP_LONG n_offset_x = FP64_Sub(g_Midpoint, speed);
        FP_LONG decay = FP64_Exp(FP64_Mul(modesConst.auxiliar_accel, n_offset_x));

        if (g_UseSmoothing) {
            FP_LONG decay_auxiliaraccel =
                    FP64_DivPrecise(decay, modesConst.auxiliar_accel);
            FP_LONG numerator = FP64_Add(
                FP64_Mul(modesConst.exp_sub_1, FP64_Sub(decay_auxiliaraccel, n_offset_x)),
                modesConst.auxiliar_constant);
            speed = FP64_Add(FP64_DivPrecise(numerator, speed), FP64_1);
        } else {
            speed = FP64_Add(
                FP64_Mul(modesConst.exp_sub_1, (FP64_Sub(
                             FP64_1, FP64_DivPrecise(FP64_Sub(g_Midpoint, FP64_Mul(decay, n_offset_x)), speed)))),
                FP64_1);
        }
    }

    return speed;
}

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

FP_LONG accel_lut(FP_LONG speed) {
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

        int index = MIN(best_point-1, g_LutSize-2);

        FP_LONG p = g_LutData_y[index];
        FP_LONG p1 = g_LutData_y[index + 1];

        // denominator should not possibly ever be equal to 0 here... (we all know how this will end)
        FP_LONG frac = FP64_DivPrecise(speed - g_LutData_x[index],
                                       g_LutData_x[index + 1] - g_LutData_x[index]);

        speed = FP64_Lerp(p, p1, frac);
    }

    return speed;
}
