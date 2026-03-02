#include <cmath>

#include "FunctionHelper.h"

#define EXP_ARG_THRESHOLD 16ll
#define FUNC_EVAL_START_VAL 0.01f

CachedFunction::CachedFunction(float xStride, Parameters *params)
        : x_stride(xStride), params(params) { }


inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

/* Parameter mapping (Rawaccel -> YeetMouse):
 * smooth -> midpoint
 * sync_speed -> accel
 * gamma -> exponent
 */
float CachedFunction::SynchronousLegacy(float x) const {
    float logMot = std::log(params->motivity);
    float gammaConst = params->exponent / logMot;
    float logSync = std::log(params->accel);

    float sharpness = (params->midpoint == 0.0) ? 16.0 : (0.5 / params->midpoint);
    float sharpnessRecip = 1.0 / sharpness;
    bool useClamp = sharpness >= 16.0;

    float minSens = 1.0 / params->motivity;
    float maxSens = params->motivity;

    if (useClamp) {
        float L = gammaConst * (std::log(x) - logSync);
        if (L < -1.0) return minSens;
        if (L > +1.0) return maxSens;
        return std::exp(L * logMot);
    }
    if (x == params->accel) {
        return 1.0;
    }

    float delta = std::log(x) - logSync;
    float M = gammaConst * std::abs(delta);
    float T = std::tanh(std::pow(M, sharpness));
    float exponent = std::pow(T, sharpnessRecip);
    if (delta < 0.0) exponent = -exponent;
    return std::exp(exponent * logMot);
}

bool CachedFunction::SynchronousBuildLUT() {
    synchronous_data.data.clear();
    synchronous_data.data.reserve(SynchronousData::capacity);
    synchronous_data.xStart = std::scalbn(1.0, SynchronousData::start);

    // integrate synchronousLegacy in small steps:
    float sum = 0.0;
    float a = 0.0;

    for (int e = 0; e < SynchronousData::stop - SynchronousData::start; ++e) {
        float expScale = std::scalbn(1.0, e + SynchronousData::start) / static_cast<float>(SynchronousData::num);
        for (int i = 0; i < SynchronousData::num; ++i) {
            float b = (i + SynchronousData::num) * expScale;
            // integrate from a -> b in two equal partitions:
            float interval = (b - a) / 2.0;
            for (int p = 1; p <= 2; ++p) {
                float xi = a + p * interval;
                sum += SynchronousLegacy(xi) * interval;
            }
            a = b;
            synchronous_data.data.push_back(!SynchronousData::velocity ? (sum / b) : sum);
        }
    }
    // final point at 2^stop:
    {
        float b = std::scalbn(1.0, SynchronousData::stop);
        float interval = (b - a) / 2.0;
        for (int p = 1; p <= 2; ++p) {
            float xi = a + p * interval;
            sum += SynchronousLegacy(xi) * interval;
        }
        a = b;
        synchronous_data.data.push_back(sum);
    }
    return true;
}

float CachedFunction::SynchronousGainEval(float x) const {
    // find exponent e = floor(log2(x)), clamped
    int e = std::min(std::max(std::ilogb(x), SynchronousData::start), SynchronousData::stop - 1);

    // fractional part in [0,1)
    float frac = std::scalbn(x, -e) - 1.0;
    float idxF = SynchronousData::num * ((e - SynchronousData::start) + frac);

    int idx = std::min(static_cast<int>(std::floor(idxF)), SynchronousData::capacity - 2);

    float y;
    if (idx >= 0 && idx < SynchronousData::capacity - 1) {
        y = lerp(synchronous_data.data[idx], synchronous_data.data[idx + 1], idxF - idx);
        if (SynchronousData::velocity) {
            y /= x;
        }
        return y;
    }

    // fallback to the first entry
    y = synchronous_data.data[0];
    if (SynchronousData::velocity) {
        y /= synchronous_data.xStart;
    }
    return y;
}

float CachedFunction::EvalFuncAt(float x) const {
    static_assert(AccelMode_Count == 10);

    x *= params->preScale;
    if (params->inCap > 0) {
        x = fminf(x, params->inCap);
    }
    float val = 0;
    switch (params->accelMode) {
        case AccelMode_Current: {
            break;
        }
        case AccelMode_Linear: // Linear
        {
            if (params->useSmoothing) {
                // The sign is used to have the possibility to
                // allow negative values
                float sign = 1.0;
                float cap_y = params->midpoint - 1.0;
                float cap_x = 0.0;
                if (cap_y != 0.0) {
                    if (cap_y < 0.0) {
                        cap_y = -cap_y;
                        sign = -sign;
                    }
                    cap_x = (cap_y / 2) / params->accel;
                }
                // The following expresions has been simplified
                // to a single constant expresion
                // float m = cap_y / 2;
                // float constant = (m - cap_y) * cap_x;
                float constant = -cap_y * cap_x / 2;
                if (x < cap_x) {
                    val = sign * x * params->accel + 1.0;
                } else {
                    val = sign * (constant / x + cap_y) + 1.0;
                }
            } else {
                val = params->accel * x + 1;
            }
            break;
        }
        case AccelMode_Power: // Power
        {
            if (x <= offset_x)
                val = params->midpoint;
            else {
                if (params->useSmoothing) {
                    float cap_y = params->motivity;
                    float cap_x = 0.0;
                    if (cap_y > 0.0) {
                        cap_x = (std::pow(cap_y / (params->exponent + 1.0), 1.0 / params->exponent)) / params->accel;
                    }
                    // float m = std::pow(cap_x * params->accel,
                    // params->exponent) + (power_constant / cap_x); float
                    // constant = (m - cap_y) * cap_x;
                    // those expresions were simplified into:
                    float constant = std::pow(cap_x * params->accel, params->exponent) * cap_x + power_constant - cap_x
                                     * cap_y;
                    if (x < cap_x) {
                        val = std::pow(x * params->accel, params->exponent) + (power_constant / x);
                    } else {
                        val = constant / x + cap_y;
                    }
                } else {
                    val = std::pow(x * params->accel, params->exponent) + (power_constant / x);
                }
            }
            //val = std::pow(x * params->accel, params->exponent) + (((std::pow(params->midpoint / (params->exponent + 1), 1 / params->exponent) / params->accel) * params->midpoint * params->exponent / (params->exponent + 1)) / x);

            break;
        }
        case AccelMode_Classic: // Classic
        {
            if (params->useSmoothing) {
                // The sign is used to have the possibility to
                // allow negative values
                float sign = 1.0;
                float accel_raised = std::pow(params->accel, params->exponent - 1.0);
                float cap_y = params->midpoint - 1.0;
                float cap_x = 0.0;
                if (cap_y != 0.0) {
                    if (cap_y < 0.0) {
                        cap_y = -cap_y;
                        sign = -sign;
                    }
                    cap_x = (std::pow(cap_y / params->exponent, 1.0 / (params->exponent - 1.0))) / params->accel;
                }
                float m = accel_raised * std::pow(cap_x, params->exponent - 1.0);
                float constant = (m - cap_y) * cap_x;
                if (x < cap_x) {
                    val = sign * std::pow(x * params->accel, params->exponent - 1.0) + 1.0;
                } else {
                    val = sign * (constant / x + cap_y) + 1.0;
                }
            } else {
                val = std::pow(x * params->accel, params->exponent - 1.0) + 1.0;
            }
            break;
        }
        case AccelMode_Motivity: // Motivity
        {
            val = (params->accel - 1) / (1 + std::exp(params->midpoint - x)) + 1;
            break;
        }
        case AccelMode_Synchronous: {
            if (params->useSmoothing) {
                val = SynchronousGainEval(x);
            } else {
                val = SynchronousLegacy(x);
            }
            break;
        }
        case AccelMode_Natural: {
            if (x <= params->midpoint) {
                val = 1;
            } else {
                float limit = params->exponent - 1.0;
                float auxiliar_accel = params->accel / std::fabs(limit);
                float offset = params->midpoint;
                float n_offset_x = offset - x;
                float decay = std::exp(auxiliar_accel * n_offset_x);

                if (params->useSmoothing) {
                    float auxiliar_constant = -limit / auxiliar_accel;
                    float numerator =
                            limit * ((decay / auxiliar_accel) - n_offset_x) +
                            auxiliar_constant;
                    val = (numerator / x) + 1.0;
                } else {
                    val = limit * (1.0 - (offset - decay * n_offset_x) / x) + 1.0;
                }
            }
            break;
        }
        case AccelMode_Jump: // Jump
        {
            if (x <= 0) {
                val = 1;
                break;
            }

            //
            // Might cause issues with high exponent's argument values
            double exp_param = smoothness * (params->midpoint - x); // smooth_rate * (step.x - x)
            double D = std::exp(exp_param); // exp()
            if (params->useSmoothing) {
                if (smoothness != 0) {
                    double log_val = exp_param > EXP_ARG_THRESHOLD ? exp_param : log(1 + D);
                    double integral = (params->accel - 1) * (x + (log_val / smoothness)); // (step.y * (x + log(1 + ()) / smooth_rate)
                    val = 1 + (integral - C0) / x; // 1 + (() + C) / x;
                }
                else if (x <= params->midpoint)
                    val = 1;
                else
                    val = 1 + (params->accel - 1) * (x - params->midpoint) / x;
            } else {
                if (smoothness != 0)
                    val = (params->accel - 1) / (1 + D) + 1;
                else if (x <= params->midpoint)
                    val = 1;
                else
                    val = (params->accel - 1) + 1;
            }
            break;
        }
        case AccelMode_CustomCurve:
        case AccelMode_Lut: // LUT
        {
            if (params->LUT_size == 0)
                break;

            if (x < params->LUT_data_x[0]) {
                val = params->LUT_data_y[0];
                break;
            }

            // Binary Search for the closest value smaller than x, so the n+1 value is greater than x
            int l = 0, r = params->LUT_size - 1;
            // while(l <= r) {
            //     int mid = (r + l) / 2;
            //
            //     if(x > params->LUT_data_x[mid]) {
            //         l = mid + 1;
            //     }
            //     else if(x < params->LUT_data_x[mid]) {
            //         r = mid - 1;
            //     }
            //     else { // This should never happen
            //         break;
            //     }
            // }
            //
            // int best_point = l;

            int best_point = params->LUT_size - 1;
            while (l <= r) {
                int mid = (r + l) / 2;

                if (x > params->LUT_data_x[mid]) {
                    l = mid + 1;
                } else {
                    best_point = mid;
                    r = mid - 1;
                }
            }

            int pos = std::min(best_point - 1, (int) params->LUT_size - 2);
            float p = params->LUT_data_y[(int) (pos)]; // p element
            float p1 = params->LUT_data_y[(int) (pos) + 1]; // p + 1 element
            // derived from this (lerp): frac * params->LUT_data_x[l + 1] + params->LUT_data_x[l] = x
            float frac = (x - params->LUT_data_x[pos]) / (params->LUT_data_x[pos + 1] - params->LUT_data_x[pos]);

            //printf("frac: %f\n", frac);

            // Interpolate between p and p+1 elements
            val = LERP(p, p1, frac);
            break;
        }
        default: {
            break;
        }
    }

    return ((params->outCap > 0) ? fminf(val, params->outCap) : val) * params->sens;
}

void CachedFunction::PreCacheConstants() {
    // Pre-Cache constants
    switch (params->accelMode) {
        case AccelMode_Current: {
            break;
        }
        case AccelMode_Linear: {
            break;
        }
        case AccelMode_Power: {
            offset_x = std::pow(params->midpoint / (params->exponent + 1), 1 / params->exponent) / params->accel;
            power_constant = offset_x * params->midpoint * params->exponent / (params->exponent + 1);
            //printf("offset_x = %f, constant = %f\n", offset_x, power_constant);
            break;
        }
        case AccelMode_Classic: {
            break;
        }
        case AccelMode_Motivity: {
            break;
        }
        case AccelMode_Synchronous: {
            if (params->useSmoothing)
                SynchronousBuildLUT();
            break;
        }
        case AccelMode_Natural: {
            break;
        }
        case AccelMode_Jump: {
            //printf("exp = %.2f, mid = %.2f\n", params->exponent, params->midpoint);
            double rate_inverse = params->exponent * params->midpoint;
            if (rate_inverse < 1.0)
                smoothness = 0;
            else
                smoothness = (2 * M_PI) / rate_inverse;

            double r_times_m = smoothness * params->midpoint;
            //printf("sm = %.2f\n", smoothness);
            //      step.y *                (log(        1 + exp(       smooth_rate * step.x))          / smooth_rate);
            if (smoothness == 0)
                C0 = 1;
            else if (r_times_m < EXP_ARG_THRESHOLD)
                C0 = (params->accel - 1) * (std::log(1 + std::exp(r_times_m))) / smoothness;
            else
                C0 = (params->accel - 1) * r_times_m / smoothness;
            break;
        }
        case AccelMode_Lut: {
            break;
        }
        case AccelMode_CustomCurve: {
            break;
        }
        default: {
            break;
        }
    }
}

void CachedFunction::PreCacheFunc() {
    PreCacheConstants();

    float x = -params->offset + FUNC_EVAL_START_VAL;
    for (int i = 0; i < PLOT_POINTS; i++) {
        if (x < 0) {
            // skip offset
            values[i] = EvalFuncAt(FUNC_EVAL_START_VAL);
            values_y[i] = params->sensY * values[i];
            x += x_stride;
            continue;
        }
        float val = EvalFuncAt(x);
        values[i] = val; // fabsf(params->outCap) > 0.01 ? fminf(val, params->outCap) : val;
        if (params->use_anisotropy)
            values_y[i] = val * params->sensY;
        x += x_stride;
    }

    ValidateSettings();
}

float CachedFunction::EvaluateFuncWithGlobalParameters(float speed) const {
    if (float x = speed - params->offset + FUNC_EVAL_START_VAL; x <= 0)
        return EvalFuncAt(FUNC_EVAL_START_VAL);
    else
        return EvalFuncAt(x);
}


bool CachedFunction::ValidateSettings() {
    isValid = true;

    for (int i = 0; i < PLOT_POINTS; i++) {
        if (std::isnan(values[i]) || std::isinf(values[i]) || values[i] > 1e5 || (
                params->use_anisotropy && (std::isnan(values_y[i]) || std::isinf(values_y[i]) || values_y[i] > 1e5))) {
            isValid = false;
            return isValid;
        }
    }

    if (params->exponent <= 0 && params->accelMode != AccelMode_Jump)
        isValid = false;

    if (params->accel < 0 || (params->accelMode != AccelMode_Linear && params->accel <= 0))
        isValid = false;

    if (params->midpoint < 0)
        isValid = false;

    if (params->accelMode == AccelMode_Lut || params->accelMode == AccelMode_CustomCurve) {
        if (params->LUT_size <= 1) {
            printf("LUT size is not valid!\n");
            isValid = false;
            return isValid;
        }
        for (int i = 0; i < params->LUT_size; i++) {
            if (std::isnan(params->LUT_data_x[i]) || std::isnan(params->LUT_data_y[i])) {
                printf("LUT data is not valid!\n");
                isValid = false;
                return isValid;
            }
        }
        // Check if is sorted
        for (int i = 1; i < params->LUT_size; i++) {
            if (params->LUT_data_x[i-1] > params->LUT_data_x[i]) {
                printf("LUT is not sorted!\n");
                isValid = false;
                return isValid;
            }
        }
    }

    if (params->accelMode == AccelMode_CustomCurve) {
        isValid = params->customCurve.points.size() >= 2;
        if (!isValid)
            return isValid;
    }

    if (params->accelMode == AccelMode_Classic) {
        if (params->useSmoothing && (params->exponent == 0 || params->exponent - 1 == 0)) {
            isValid = false;
        }
    }

    if (params->accelMode == AccelMode_Power) {
        if (std::pow(params->midpoint / (params->exponent + 1), 1 / params->exponent) / params->accel > 1e8) {
            isValid = false;
        }

        if (params->useSmoothing && (params->motivity <= params->midpoint)) {
            isValid = false;
        }

        if (std::isnan(power_constant) || std::isinf(power_constant) || std::isnan(offset_x) || std::isinf(offset_x)) {
            isValid = false;
        }
    }

    if (params->accelMode == AccelMode_Jump) {
        if (params->midpoint <= 0)
            isValid = false;

        if (std::isnan(smoothness) || std::isinf(smoothness) || std::isnan(C0) || std::isinf(C0)) {
            isValid = false;
        }
    }

    if (params->accelMode == AccelMode_Natural) {
        if (params->midpoint < 0) {
            isValid = false;
        }
    }

    if (params->accelMode == AccelMode_Synchronous) {
        if (params->motivity <= 1) {
            isValid = false;
        }
    }

    return isValid;
}
