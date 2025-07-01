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
    speed = FP64_Mul(speed, g_Acceleration);
    speed = FP64_PowFast(speed, modesConst.exp_sub_1);
    speed = FP64_Add(speed, FP64_1);
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
	FP_LONG limit = FP64_Sub(g_Exponent, FP64_1);
        FP_LONG auxiliar_accel =
            FP64_DivPrecise(g_Acceleration, FP64_Abs(limit));
        FP_LONG n_offset_x = FP64_Sub(g_Midpoint, speed);
        FP_LONG decay = FP64_Exp(FP64_Mul(auxiliar_accel, n_offset_x));

        if (g_UseSmoothing) {
            FP_LONG auxiliar_constant = FP64_DivPrecise(-limit, auxiliar_accel);
            FP_LONG decay_auxiliaraccel =
                FP64_DivPrecise(decay, auxiliar_accel);
            FP_LONG numerator = FP64_Add(
                FP64_Mul(limit, FP64_Sub(decay_auxiliaraccel, n_offset_x)),
					 auxiliar_constant);
            speed = FP64_Add(FP64_DivPrecise(numerator, speed), FP64_1);
	} else {
	    speed = FP64_Add(FP64_Mul(limit, (FP64_Sub(FP64_1, FP64_DivPrecise(FP64_Sub(g_Midpoint, FP64_Mul(decay, n_offset_x)), speed)))), FP64_1);
	}
    }

    return speed;
}

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
