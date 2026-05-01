#include "accel_modes.h"

#include "FixedMath/Fixed64.h"
#include "FixedMath/FixedUtil.h"
#include "accel.h"

#define EXP_ARG_THRESHOLD 16ll

static FP_LONG synchronous_legacy(const struct ModesConstants *constants, FP_LONG acceleration, FP_LONG x) {
    if (constants->useClamp) {
        FP_LONG L = FP64_Mul(constants->gammaConst, FP64_Sub(FP64_Log(x), constants->logSync));
        if (L < FP64_1) return constants->minSens;
        if (L > -FP64_1) return constants->maxSens;
        return FP64_Exp(FP64_Mul(L, constants->logMot));
    }

    if (x == acceleration) {
        return FP64_1;
    }

    FP_LONG delta = FP64_Sub(FP64_Log(x), constants->logSync);
    FP_LONG M = FP64_Mul(constants->gammaConst, FP64_Abs(delta));
    FP_LONG T = FP64_Tanh(FP64_Pow(M, constants->sharpness));
    FP_LONG exponent = FP64_Pow(T, constants->sharpnessRecip);
    if (delta < 0) {
        exponent = -exponent;
    }
    return FP64_Exp(FP64_Mul(exponent, constants->logMot));
}

// Helper: build LUT for smoothing/gain mode
static bool synchronous_build_lut(struct ModesConstants *constants, FP_LONG acceleration) {
    // x_start = 2^SYNC_START
    constants->x_start = FP64_Scalbn(FP64_1, SYNC_START);

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
                sum = FP64_Add(sum, FP64_Mul(synchronous_legacy(constants, acceleration, xi), interval));
            }

            prev_x = b;

            constants->data[idx++] = sum;
        }
    }

    // final point at 2^SYNC_STOP
    {
        FP_LONG b = FP64_Scalbn(FP64_1, SYNC_STOP);
        FP_LONG interval = FP64_DivPrecise(FP64_Sub(b, prev_x), FP64_FromInt(2));
        for (int p = 1; p <= 2; ++p) {
            FP_LONG xi = FP64_Add(prev_x, FP64_Mul(FP64_FromInt(p), interval));
            sum = FP64_Add(sum, FP64_Mul(synchronous_legacy(constants, acceleration, xi), interval));
        }
        prev_x = b;

        if (idx < SYNC_CAPACITY) {
            constants->data[idx] = sum; // last element
        }
    }

    constants->lut_ready = true;
    return true;
}

// Recalculate new modes constants
void update_constants(struct accel_params *params, struct ModesConstants *constants) {
    // General
    constants->accel_sub_1 = FP64_Sub(params->acceleration, FP64_1);
    constants->exp_sub_1 = FP64_Sub(params->exponent, FP64_1);
    constants->cap_x = 0;
    constants->cap_y = 0;
    constants->gain_constant = 0;
    constants->sign = FP64_1;

    // Synchronous
    if (params->acceleration_mode == AccelMode_Synchronous) {
        if (params->motivity <= FP64_1) {
            printk("YeetMouse: Error: Acceleration mode 'Synchronous' is not supported for motivity 1.\n");
            params->acceleration = 0;
            params->acceleration_mode = AccelMode_Current;
        }
        else {
            constants->logMot = FP64_Log(params->motivity);
            constants->gammaConst = FP64_DivPrecise(params->exponent, constants->logMot);
            constants->logSync = FP64_Log(params->acceleration);

            // sharpness = (midpoint == 0) ? 16.0 : (0.5 / midpoint)
            constants->sharpness = (params->midpoint == 0)
                ? FP64_FromInt(16)
                : FP64_DivPrecise(FP64_0_5, params->midpoint);

            constants->sharpnessRecip = FP64_DivPrecise(FP64_1, constants->sharpness);
            constants->useClamp = (constants->sharpness >= FP64_FromInt(16));

            constants->minSens = FP64_DivPrecise(FP64_1, params->motivity);
            constants->maxSens = params->motivity;

            constants->lut_ready = false;
        }

        if (params->use_smoothing) {
            synchronous_build_lut(constants, params->acceleration);
        }
    }

    // Linear
    if (params->acceleration_mode == AccelMode_Linear) {
        if (params->acceleration == 0) {
            printk("YeetMouse: Error: Acceleration mode 'Linear' is not supported for acceleration 0.\n");
            params->acceleration = 0;
            params->acceleration_mode = AccelMode_Current;
        }
        else if (params->use_smoothing) {
            FP_LONG sign = FP64_1;
            FP_LONG cap_y = FP64_Sub(params->midpoint, FP64_1);
            FP_LONG cap_x = FP64_FromInt(0);
            FP_LONG constant = FP64_FromInt(0);
            if (cap_y != 0) {
                if (cap_y < 0) {
                    cap_y = FP64_Mul(cap_y, Neg1);
                    sign = Neg1;
                }
                cap_x = FP64_DivPrecise(FP64_DivPrecise(cap_y, FP64_FromInt(2)), params->acceleration);
            }
            constant = FP64_DivPrecise(FP64_Mul(FP64_Mul(cap_y, Neg1), cap_x), FP64_FromInt(2));
            constants->cap_x = cap_x;
            constants->cap_y = cap_y;
            constants->gain_constant = constant;
            constants->sign = sign;
        }
    }

    // Classic
    if (params->acceleration_mode == AccelMode_Classic) {
        if (params->use_smoothing && (params->exponent == 0 || constants->exp_sub_1 == 0)) {
            printk("YeetMouse: Error: Acceleration mode 'Classic' is not supported for exponent 0 or 1 while using the the smooth cap.\n");
            params->acceleration = 0;
            params->acceleration_mode = AccelMode_Current;
        } else {
            if (params->use_smoothing) {
                FP_LONG sign = FP64_1;
                FP_LONG cap_y = FP64_Sub(params->midpoint, FP64_1);
                FP_LONG cap_x = FP64_FromInt(0);
                FP_LONG constant = FP64_FromInt(0);
                if (cap_y != 0) {
                    if (cap_y < 0) {
                        cap_y = FP64_Mul(cap_y, Neg1);
                        sign = Neg1;
                    }
                    cap_x = FP64_DivPrecise(FP64_Pow(FP64_DivPrecise(cap_y, params->exponent),
                                                     FP64_DivPrecise(FP64_1, constants->exp_sub_1)), params->acceleration);
                }
                FP_LONG factor = FP64_DivPrecise(FP64_Sub(params->exponent, FP64_1), params->exponent);
                constant = FP64_Mul(cap_y, cap_x);
                constant = FP64_Mul(factor, constant);
                constant = FP64_Mul(constant, Neg1);
                constants->cap_x = cap_x;
                constants->cap_y = cap_y;
                constants->gain_constant = constant;
                constants->sign = sign;
            }
        }
    }

    // Natural
    if (params->acceleration_mode == AccelMode_Natural) {
        if (constants->exp_sub_1 == 0 || params->exponent == FP64_1) {
            printk("YeetMouse: Error: Acceleration mode 'Natural' is not supported for exponent 1.\n");
            params->acceleration = 0;
            params->acceleration_mode = AccelMode_Current;
        }
        if (params->acceleration == 0) {
            printk("YeetMouse: Error: Acceleration mode 'Natural' is not supported for acceleration 0.\n");
            params->acceleration = 0;
            params->acceleration_mode = AccelMode_Current;
        }
        else {
            constants->auxiliar_accel = FP64_DivPrecise(params->acceleration, FP64_Abs(constants->exp_sub_1));
            constants->auxiliar_constant = FP64_DivPrecise(-constants->exp_sub_1, constants->auxiliar_accel);
        }
    }

    // Jump
    if (params->acceleration_mode == AccelMode_Jump) {
        if (params->midpoint == 0) {
            printk("YeetMouse: Error: Acceleration mode 'Jump' is not supported for midpoint 0.\n");
            params->midpoint = FP64_1;
            params->acceleration = 0;
            params->acceleration_mode = AccelMode_Current;
        }
        else {
            FP_LONG smooth_inv = FP64_Mul(params->exponent, params->midpoint);
            if (smooth_inv < FP64_1)
                constants->r = 0;
            else
                constants->r = FP64_DivPrecise(Pi2, smooth_inv);

            FP_LONG r_times_m = FP64_Mul(constants->r, params->midpoint);

            if (constants->r == 0) {
                constants->C0 = FP64_1;
            }
            // Safely exponentiate without overflow (ln(1+exp(x)) when x -> 'inf' = ln(exp(x)) = x. (in practice works for x >= 8))
            else if (r_times_m < (EXP_ARG_THRESHOLD << FP64_Shift))
                constants->C0 = FP64_Mul(constants->accel_sub_1, FP64_DivPrecise(FP64_Log(FP64_Add(FP64_1, FP64_Exp(r_times_m))), constants->r));
            else
                constants->C0 = FP64_Mul(constants->accel_sub_1, FP64_DivPrecise(r_times_m, constants->r));
        }
    }

    // Power
    if (params->acceleration_mode == AccelMode_Power) {
        if (params->exponent == 0 || params->exponent == -FP64_1 || params->acceleration == 0) {
            printk("YeetMouse: Error: Acceleration mode 'Power' is not supported for exponent 0 or -1 or acceleration 0.\n");
            params->acceleration = 0;
            params->acceleration_mode = AccelMode_Current;
        }
        else if (params->midpoint == 0 && !params->use_smoothing) {
            constants->offset_x = 0;
            constants->power_constant = 0;
        }
        else if ((params->midpoint >= params->motivity) && params->use_smoothing) {
            printk("YeetMouse: Error: Acceleration mode 'Power' is not supported for output offsets higher than the smooth cap.\n");
            params->acceleration = 0;
            params->acceleration_mode = AccelMode_Current;
        }
        else if (FP64_DivPrecise(params->midpoint, FP64_Mul(params->acceleration, params->exponent)) > FP64_100) { // 100 here is completely arbitrary
            printk("YeetMouse: Error: Invalid parameters for the 'Power' mode.\n");
            params->acceleration = 0;
            params->acceleration_mode = AccelMode_Current;
        }
        else {
            // constants->offset_x = FP64_DivPrecise(FP64_Pow(FP64_DivPrecise(params->midpoint, FP64_Add(params->exponent, FP64_ONE)),
            //     FP64_DivPrecise(FP64_ONE, params->exponent)), params->acceleration);
            // constants->power_constant = FP64_DivPrecise(FP64_Mul(constants->offset_x, FP64_Mul(params->midpoint, params->exponent)), FP64_Add(params->exponent, FP64_ONE));

            FP_LONG exponent_plus_one = FP64_Add(params->exponent, FP64_1);
            if (params->midpoint == 0) {
                constants->offset_x = 0;
                constants->power_constant = 0;
            } else {
                FP_LONG one_over_exponent = FP64_DivPrecise(FP64_1, params->exponent);
                FP_LONG base_value = FP64_DivPrecise(params->midpoint, exponent_plus_one);

                FP_LONG pow_result = FP64_Pow(base_value, one_over_exponent);
                constants->offset_x = FP64_DivPrecise(pow_result, params->acceleration);

                FP_LONG intermediate = FP64_Mul(constants->offset_x, FP64_Mul(params->midpoint, params->exponent));
                constants->power_constant = FP64_DivPrecise(intermediate, exponent_plus_one);
            }

            if (params->use_smoothing) {
                FP_LONG cap_y = params->motivity;
                FP_LONG cap_x = FP64_FromInt(0);
                if (cap_y > FP64_FromInt(0)) {
                  cap_x = FP64_DivPrecise(
                      FP64_Pow(
                          FP64_DivPrecise(cap_y, exponent_plus_one),
                          FP64_DivPrecise(FP64_1, params->exponent)),
                                          params->acceleration);
                }
                FP_LONG constant = FP64_Mul(params->acceleration, cap_x);
                constant = FP64_Pow(constant, params->exponent);
                constant = FP64_Mul(constant, cap_x);
                constant = FP64_Add(constant, constants->power_constant);
                constant = FP64_Sub(constant, FP64_Mul(cap_x, cap_y));

                constants->cap_x = cap_x;
                constants->cap_y = cap_y;
                constants->gain_constant = constant;
            }
        }
    }

    // Lut (Validation)
    if (params->acceleration_mode == AccelMode_Lut || params->acceleration_mode == AccelMode_CustomCurve) {
        if (params->lut_pairs <= 1 || params->lut_data_x[params->lut_pairs-1] == params->lut_data_x[params->lut_pairs-2])
            params->acceleration_mode = AccelMode_Current;

        // Check if LUT_x is sorted
        for (int i = 1; i < params->lut_pairs; i++) {
            if (params->lut_data_x[i - 1] > params->lut_data_x[i]) {
                params->acceleration_mode = AccelMode_Current;
                printk("YeetMouse: Error: Acceleration mode 'LUT' is not supported for unsorted LUT_x.\n");
                break;
            }
        }
    }

    static_assert(AccelMode_Count == 10, "Wrong AccelMode count!");
    switch (params->acceleration_mode) {
        case AccelMode_Linear:
            constants->current_func_at_0 = accel_linear(constants, params->acceleration, params->use_smoothing, FP64_0_01);
            break;
        case AccelMode_Power:
            constants->current_func_at_0 = accel_power(constants, params->midpoint, params->acceleration, params->exponent, params->use_smoothing, FP64_0_01);
            break;
        case AccelMode_Classic:
            constants->current_func_at_0 = accel_classic(constants, params->acceleration, params->use_smoothing, FP64_0_01);
            break;
        case AccelMode_Motivity:
            constants->current_func_at_0 = accel_motivity(constants, params->midpoint, FP64_0_01);
            break;
        case AccelMode_Synchronous:
            constants->current_func_at_0 = accel_synchronous(constants, params->acceleration, params->use_smoothing, FP64_0_01);
            break;
        case AccelMode_Natural:
            constants->current_func_at_0 = accel_natural(constants, params->midpoint, params->use_smoothing, FP64_0_01);
            break;
        case AccelMode_Jump:
            constants->current_func_at_0 = accel_jump(constants, params->midpoint, params->use_smoothing, FP64_0_01);
            break;
        case AccelMode_Lut: case AccelMode_CustomCurve:
            constants->current_func_at_0 = accel_lut(params->lut_pairs, params->lut_data_x, params->lut_data_y, FP64_0_01);
            break;
        default:
            constants->current_func_at_0 = FP64_1;
            break;
    }

    // Rotation (precalculate the trig. functions)
    constants->sin_a = FP64_Sin(params->rotation_angle);
    constants->cos_a = FP64_Cos(params->rotation_angle);

    constants->as_cos = FP64_Cos(params->angle_snap_angle);
    constants->as_sin = FP64_Sin(params->angle_snap_angle);
    constants->as_half_threshold = FP64_DivPrecise(params->angle_snap_threshold, 2ll << FP64_Shift);

    constants->is_init = 1;
}


static FP_LONG synchronous_eval(const struct ModesConstants *constants, FP_LONG x) {
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

        FP_LONG y = FP64_Lerp(constants->data[idx], constants->data[idx + 1], t);

        return FP64_DivPrecise(y, x);
    }
    FP_LONG y = constants->data[0];
    return FP64_DivPrecise(y, constants->x_start);
}

FP_LONG accel_linear(const struct ModesConstants *constants, FP_LONG acceleration, bool use_smoothing, FP_LONG speed) {
    if (use_smoothing) {
        if (speed < constants->cap_x) {
            speed = FP64_Mul(constants->sign, FP64_Mul(speed, acceleration));
        } else {
            speed = FP64_Mul(constants->sign, FP64_Add(FP64_DivPrecise(constants->gain_constant, speed), constants->cap_y));
        }
    } else {
        speed = FP64_Mul(speed, acceleration);
    }
    return FP64_Add(FP64_1, speed);
}

FP_LONG accel_power(const struct ModesConstants *constants, FP_LONG midpoint, FP_LONG acceleration, FP_LONG exponent, bool use_smoothing, FP_LONG speed) {
    if (speed <= constants->offset_x)
        speed = midpoint;
    else {
        if (use_smoothing) {
            if (speed < constants->cap_x) {
                if (constants->power_constant == 0)
                    speed = FP64_PowFast(FP64_Mul(speed, acceleration), exponent);
                else
                    speed = FP64_Add(FP64_PowFast(FP64_Mul(speed, acceleration), exponent), FP64_DivPrecise(constants->power_constant, speed));
            } else {
                if (constants->cap_x == FP64_FromInt(0)) {
                    speed = constants->cap_y;
                } else {
                    speed = FP64_Add(FP64_DivPrecise(constants->gain_constant, speed), constants->cap_y);
                }
            }
        } else {
            if (constants->power_constant == 0)
                speed = FP64_PowFast(FP64_Mul(speed, acceleration), exponent);
            else
                speed = FP64_Add(FP64_PowFast(FP64_Mul(speed, acceleration), exponent), FP64_DivPrecise(constants->power_constant, speed));
        }
    }
    return speed;
}

FP_LONG accel_classic(const struct ModesConstants *constants, FP_LONG acceleration, bool use_smoothing, FP_LONG speed) {
    // (Speed * Acceleration) ^ (Exponent - 1) + 1
    // Same as above just without adding the one
    //speed *= g_Acceleration;
    //speed += 1;
    //B_pow(&speed, &g_Exponent);

    // FIXED-POINT:
    FP_LONG accel_classic_result = speed;
    accel_classic_result = FP64_Mul(accel_classic_result, acceleration);
    accel_classic_result = FP64_PowFast(accel_classic_result, constants->exp_sub_1);

    // if Use Smooth Cap is on, we proceed to calculate the transition
    // point and the function that provides the smooth cap
    if (use_smoothing) {
        // we setup the y cap
        if (speed < constants->cap_x) {
            accel_classic_result = FP64_Mul(constants->sign, accel_classic_result);
            speed = FP64_Add(accel_classic_result, FP64_1);
        } else {
            speed = FP64_Add(FP64_Mul(constants->sign,
                                      FP64_Add(FP64_DivPrecise(constants->gain_constant, speed),
                                               constants->cap_y)), FP64_1);
        }
    } else
        speed = FP64_Add(accel_classic_result, FP64_1);

    return speed;
}

FP_LONG accel_motivity(const struct ModesConstants *constants, FP_LONG midpoint, FP_LONG speed) {
    // Acceleration / ( 1 + e ^ (midpoint - x))
    //product = g_Midpoint-speed;
    //motivity = e;
    //B_pow(&motivity, &product);
    //motivity = g_Acceleration / (1 + motivity);
    //speed = motivity;

    // FIXED-POINT:
    FP_LONG exp = FP64_ExpFast(FP64_Sub(midpoint, speed));
    speed = FP64_Add(FP64_1, FP64_DivPrecise(constants->accel_sub_1, FP64_Add(FP64_1, exp)));
    return speed;
}

FP_LONG accel_synchronous(const struct ModesConstants *constants, FP_LONG acceleration, bool use_smoothing, FP_LONG speed) {
    // Defensive: ensure speed > 0 for log-domain math; you can clamp differently if your file already does.
    if (speed <= 0) {
        return FP64_1;
    }

    FP_LONG val;
    if (use_smoothing && constants->lut_ready) {
        val = synchronous_eval(constants, speed);
    } else {
        val = synchronous_legacy(constants, acceleration, speed);
    }
    return val;
}


FP_LONG accel_jump(const struct ModesConstants *constants, FP_LONG midpoint, bool use_smoothing, FP_LONG speed) {
    // r = 2pi/(k*midpoint), where k is the smoothness factor (stored inside g_Exponent)
    // Jump: Acceleration / (1 + exp(r(midpoint - x))) + 1
    // Smooth: Integral of the above divided by x pretty much

    if (speed <= 0)
        return FP64_1;

    FP_LONG exp_arg = FP64_Mul(constants->r, FP64_Sub(midpoint, speed));
    FP_LONG D = FP64_Exp(exp_arg);

    if(use_smoothing) { // smooth
        if (constants->r != 0) {
            FP_LONG natural_log = exp_arg > (EXP_ARG_THRESHOLD << FP64_Shift) ? exp_arg : FP64_Log(FP64_Add(FP64_1, D));
            FP_LONG integral = FP64_Mul(constants->accel_sub_1, FP64_Add(speed, FP64_DivPrecise(natural_log, constants->r)));
            // Not really an integral
            speed = FP64_Add(FP64_DivPrecise(FP64_Sub(integral, constants->C0), speed), FP64_1);
        }
        else if (speed <= midpoint)
            speed = FP64_1;
        else
            speed = FP64_Add(FP64_DivPrecise(FP64_Mul(constants->accel_sub_1, FP64_Sub(speed, midpoint)), speed), FP64_1);
    }
    else {
        if (constants->r != 0)
            speed = FP64_Add(FP64_DivPrecise(constants->accel_sub_1, FP64_Add(FP64_1, D)), FP64_1);
        else if (speed <= midpoint)
            speed = FP64_1;
        else
            speed = FP64_Add(constants->accel_sub_1, FP64_1);
    }

    return speed;
}

FP_LONG accel_natural(const struct ModesConstants *constants, FP_LONG midpoint, bool use_smoothing, FP_LONG speed) {
    if (speed <= midpoint) {
        speed = FP64_1;
    } else {
        FP_LONG n_offset_x = FP64_Sub(midpoint, speed);
        FP_LONG decay = FP64_Exp(FP64_Mul(constants->auxiliar_accel, n_offset_x));

        if (use_smoothing) {
            FP_LONG decay_auxiliaraccel =
                    FP64_DivPrecise(decay, constants->auxiliar_accel);
            FP_LONG numerator = FP64_Add(
                FP64_Mul(constants->exp_sub_1, FP64_Sub(decay_auxiliaraccel, n_offset_x)),
                constants->auxiliar_constant);
            speed = FP64_Add(FP64_DivPrecise(numerator, speed), FP64_1);
        } else {
            speed = FP64_Add(
                FP64_Mul(constants->exp_sub_1, (FP64_Sub(
                             FP64_1, FP64_DivPrecise(FP64_Sub(midpoint, FP64_Mul(decay, n_offset_x)), speed)))),
                FP64_1);
        }
    }

    return speed;
}

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

FP_LONG accel_lut(unsigned long lut_pairs, const FP_LONG lut_data_x[MAX_LUT_ARRAY_SIZE], const FP_LONG lut_data_y[MAX_LUT_ARRAY_SIZE], FP_LONG speed) {
    // Assumes the size and values are valid. Please don't change LUT parameters by hand.

    if(speed < lut_data_x[0]) // Check if the speed is below the first given point
        speed = lut_data_y[0];
    else {
        int l = 0, r = lut_pairs - 1, best_point = r, iter = 0; // We REALLY don't want an infinity loop in kernel
        while (l <= r && iter < 10) {
            int mid = (r + l) / 2;

            if (speed > lut_data_x[mid]) {
                l = mid + 1;
            } else {
                best_point = mid;
                r = mid - 1;
            }

            iter++;
        }

        int index = MIN(best_point-1, lut_pairs-2);

        FP_LONG p = lut_data_y[index];
        FP_LONG p1 = lut_data_y[index + 1];

        // denominator should not possibly ever be equal to 0 here... (we all know how this will end)
        FP_LONG frac = FP64_DivPrecise(speed - lut_data_x[index],
                                       lut_data_x[index + 1] - lut_data_x[index]);

        speed = FP64_Lerp(p, p1, frac);
    }

    return speed;
}
