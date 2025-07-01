#include <cmath>

#include "FunctionHelper.h"

CachedFunction::CachedFunction(float xStride, Parameters *params)
        : x_stride(xStride), params(params) { }

float CachedFunction::EvalFuncAt(float x) {
    x *= params->preScale;
    if(params->inCap > 0) {
        x = fminf(x, params->inCap);
    }
    float val = 0;
    switch (params->accelMode) {
        case AccelMode_Current:
        {
            break;
        }
        case AccelMode_Linear: // Linear
        {
            val = params->accel * x + 1;
            break;
        }
        case AccelMode_Power: // Power
        {
            if (x <= offset_x)
                val = params->midpoint;
            else
                val = std::pow(x * params->accel, params->exponent) + (power_constant / x);

            //val = std::pow(x * params->accel, params->exponent) + (((std::pow(params->midpoint / (params->exponent + 1), 1 / params->exponent) / params->accel) * params->midpoint * params->exponent / (params->exponent + 1)) / x);

            break;
        }
        case AccelMode_Classic: // Classic
        {
            val = std::pow(x * params->accel, params->exponent - 1) + 1;
            break;
        }
        case AccelMode_Motivity: // Motivity
        {
            val = (params->accel - 1) / (1 + std::exp(params->midpoint - x)) + 1;
            break;
        }
        case AccelMode_Natural:
        {
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
            // Might cause issues with high exponent's argument values
            double exp_param = smoothness * (params->midpoint - x);
            double D = std::exp(exp_param);
            if(params->useSmoothing) {
                double integral = (params->accel - 1) * (x + (log(1 + D) / smoothness));
                val = ((integral - C0) / x) + 1;
            }
            else {
                val = (params->accel - 1) / (1 + D) + 1;
            }
            break;
        }
        case AccelMode_CustomCurve:
        case AccelMode_Lut: // LUT
        {
            if(params->LUT_size == 0)
                break;

            if(x < params->LUT_data_x[0]) {
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

            int pos = std::min(best_point-1, (int)params->LUT_size - 2);
            float p = params->LUT_data_y[(int)(pos)]; // p element
            float p1 = params->LUT_data_y[(int)(pos) + 1]; // p + 1 element
            // derived from this (lerp): frac * params->LUT_data_x[l + 1] + params->LUT_data_x[l] = x
            float frac = (x - params->LUT_data_x[pos]) / (params->LUT_data_x[pos + 1] - params->LUT_data_x[pos]);

            //printf("frac: %f\n", frac);

            // Interpolate between p and p+1 elements
            val = LERP(p, p1, frac);
            break;
        }
        default:
        {
            break;
        }
    }

    return ((params->outCap > 0) ? fminf(val, params->outCap) : val) * params->sens;
}

void CachedFunction::PreCacheConstants() {
    // Pre-Cache constants
    switch (params->accelMode) {
        case AccelMode_Current:
        {
            break;
        }
        case AccelMode_Linear:
        {
            break;
        }
        case AccelMode_Power:
        {
            offset_x = std::pow(params->midpoint / (params->exponent + 1), 1 / params->exponent) / params->accel;
            power_constant = offset_x * params->midpoint * params->exponent / (params->exponent + 1);
            //printf("offset_x = %f, constant = %f\n", offset_x, power_constant);
            break;
        }
        case AccelMode_Classic:
        {
            break;
        }
        case AccelMode_Motivity:
        {
            break;
        }
        case AccelMode_Natural:
        {
	break;
	}
        case AccelMode_Jump:
        {
            //printf("exp = %.2f, mid = %.2f\n", params->exponent, params->midpoint);
            smoothness = (2 * M_PI) / (params->exponent * params->midpoint);
            //printf("sm = %.2f\n", smoothness);
            //C0 = params->accel * params->midpoint; // Fast approx
            C0 = (params->accel - 1) * (smoothness * params->midpoint + std::log(1+std::exp(-smoothness * params->midpoint)))/smoothness;
            break;
        }
        case AccelMode_Lut:
        {
            break;
        }
        case AccelMode_CustomCurve:
        {
            break;
        }
        default:
        {
            break;
        }
    }
}

void CachedFunction::PreCacheFunc() {
    PreCacheConstants();

    float x = -params->offset + 0.01;
    for(int i = 0; i < PLOT_POINTS; i++) {
        if(x < 0) { // skip offset
            values[i] = params->sens;
            values_y[i] = params->sensY;
            x += x_stride;
            continue;
        }
        float val = EvalFuncAt(x);
        values[i] = val; // fabsf(params->outCap) > 0.01 ? fminf(val, params->outCap) : val;
        if (params->use_anisotropy)
            values_y[i] = val / params->sens * params->sensY;
        x += x_stride;
    }

    ValidateSettings();
}


bool CachedFunction::ValidateSettings() {
    isValid = true;

    for (int i = 0; i < PLOT_POINTS; i++) {
        if (std::isnan(values[i]) || std::isnan(values_y[i]) || std::isinf(values[i]) || std::isinf(values_y[i]) || values[i] > 1e5 || values_y[i] > 1e5) {
            isValid = false;
            return isValid;
        }
    }

    if (params->exponent <= 0)
        isValid = false;

    if (params->accel <= 0)
        isValid = false;

    if (params->midpoint < 0)
        isValid = false;

    if (params->accelMode == AccelMode_Lut) {
        for (int i = 0; i < MAX_LUT_ARRAY_SIZE; i++) {
            if (std::isnan(params->LUT_data_x[i]) || std::isnan(params->LUT_data_y[i])) {
                isValid = false;
                return isValid;
            }
        }
    }

    if (params->accelMode == AccelMode_Power) {
        if (std::pow(params->midpoint / (params->exponent + 1), 1 / params->exponent) / params->accel > 1e8) {
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

    return isValid;
}
