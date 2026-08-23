#include "TestManager.h"

#include "shared_definitions.h"
#include "driver/accel_modes.h"

// "Private" values only visible to the accel_modes
accel_params accelParams;
ModesConstants modesConst;
static CachedFunction function;

// Ignores speedY (for now?)
FP_LONG ApplyGlobalPostParameters(FP_LONG speed) {
    FP_LONG speed_Y = FP64_1;
    if (accelParams.ratio_yx == FP64_1) {
        if(accelParams.sensitivity != FP64_1)
            speed = FP64_Mul(speed, accelParams.sensitivity);

        // Apply Output Limit
        if(accelParams.output_cap > 0)
            speed = FP64_Min(accelParams.output_cap, speed);
    } else {
        speed = FP64_Mul(speed, accelParams.sensitivity);
        speed_Y = FP64_Mul(speed, accelParams.ratio_yx);

        // Apply Output Limit
        if(accelParams.output_cap > 0) {
            speed = FP64_Min(accelParams.output_cap, speed);
            speed_Y = FP64_Min(accelParams.output_cap, speed_Y);
        }
    }

    return speed;
}

FP_LONG ApplyGlobalPreParameters(FP_LONG speed) {
    return FP64_Mul(speed, accelParams.prescale);
}

// TestManager & TestManager::GetInstance() {
//     static TestManager instance;
//     return instance;
// }

void TestManager::Initialize() {
    function.params = new Parameters;
    function.params->sens = FP64_ToFloat(accelParams.sensitivity);
    function.params->ratioYX = FP64_ToFloat(accelParams.ratio_yx);
    function.params->accelMode = static_cast<AccelMode>(accelParams.acceleration_mode);
    function.params->preScale = FP64_ToFloat(accelParams.prescale);
    function.params->accel = FP64_ToFloat(accelParams.acceleration);
    function.params->exponent = FP64_ToFloat(accelParams.exponent);
    function.params->midpoint = FP64_ToFloat(accelParams.midpoint);
    function.params->offset = FP64_ToFloat(accelParams.offset);
    function.params->useSmoothing = accelParams.use_smoothing;
    function.params->rotation = FP64_ToFloat(accelParams.rotation_angle);
    function.params->asAngle = FP64_ToFloat(accelParams.angle_snap_angle);
    function.params->asThreshold = FP64_ToFloat(accelParams.angle_snap_threshold);
    function.params->inCap = 0;
    function.params->outCap = 0;
    function.PreCacheConstants();
}

FP_LONG TestManager::AccelLinear(FP_LONG x, FP_LONG acceleration, FP_LONG midpoint, bool gain) {
    SetAcceleration(acceleration);
    SetUseSmoothing(gain);
    SetMidpoint(midpoint);
    UpdateModesConstants();
    return ApplyGlobalPostParameters(accel_linear(&modesConst, &accelParams, ApplyGlobalPreParameters(x)));
}

FP_LONG TestManager::AccelPower(FP_LONG x, FP_LONG acceleration, FP_LONG exponent, FP_LONG midpoint, FP_LONG motivity,
                                bool gain) {
    SetAcceleration(acceleration);
    SetExponent(exponent);
    SetMidpoint(midpoint);
    SetMotivity(motivity);
    SetUseSmoothing(gain);
    UpdateModesConstants();
    return ApplyGlobalPostParameters(accel_power(&modesConst, &accelParams, ApplyGlobalPreParameters(x)));
}

FP_LONG TestManager::AccelClassic(FP_LONG x, FP_LONG acceleration, FP_LONG exponent, FP_LONG midpoint, bool gain) {
    SetAcceleration(acceleration);
    SetExponent(exponent);
    SetMidpoint(midpoint);
    SetUseSmoothing(gain);
    UpdateModesConstants();
    return ApplyGlobalPostParameters(accel_classic(&modesConst, &accelParams, ApplyGlobalPreParameters(x)));
}

FP_LONG TestManager::AccelMotivity(FP_LONG x, FP_LONG acceleration, FP_LONG exponent, FP_LONG midpoint) {
    SetAcceleration(acceleration);
    SetExponent(exponent);
    SetMidpoint(midpoint);
    UpdateModesConstants();
    return ApplyGlobalPostParameters(accel_motivity(&modesConst, &accelParams, ApplyGlobalPreParameters(x)));
}

FP_LONG TestManager::AccelSynchronous(FP_LONG x, FP_LONG sync_speed, FP_LONG gamma, FP_LONG smoothness,
                                      FP_LONG motivity, bool gain) {
    SetAcceleration(sync_speed);
    SetExponent(gamma);
    SetMidpoint(smoothness);
    SetMotivity(motivity);
    SetUseSmoothing(gain);
    UpdateModesConstants();
    return ApplyGlobalPostParameters(accel_synchronous(&modesConst, &accelParams, ApplyGlobalPreParameters(x)));
}

FP_LONG TestManager::AccelJump(FP_LONG x, FP_LONG acceleration, FP_LONG exponent, FP_LONG midpoint, bool gain) {
    SetAcceleration(acceleration);
    SetExponent(exponent);
    SetMidpoint(midpoint);
    SetUseSmoothing(gain);
    UpdateModesConstants();
    return ApplyGlobalPostParameters(accel_jump(&modesConst, &accelParams, ApplyGlobalPreParameters(x)));
}

FP_LONG TestManager::AccelLUT(FP_LONG x, FP_LONG values_x[], FP_LONG values_y[], unsigned long count) {
    SetLutSize(count);
    SetLutData_x(values_x, count);
    SetLutData_y(values_y, count);
    UpdateModesConstants();
    return ApplyGlobalPostParameters(accel_lut(&accelParams, ApplyGlobalPreParameters(x)));
}

FP_LONG TestManager::AccelLUT(FP_LONG x) {
    return ApplyGlobalPostParameters(accel_lut(&accelParams, ApplyGlobalPreParameters(x)));
}

FP_LONG TestManager::AccelLinear(float x, float acceleration, float midpoint, bool gain) {
    return AccelLinear(FP64_FromFloat(x), FP64_FromFloat(acceleration), FP64_FromFloat(midpoint), gain);
}

FP_LONG TestManager::AccelPower(float x, float acceleration, float exponent, float midpoint, float motivity,
                                bool gain) {
    return AccelPower(FP64_FromFloat(x), FP64_FromFloat(acceleration), FP64_FromFloat(exponent),
                      FP64_FromFloat(midpoint), FP64_FromFloat(motivity), gain);
}

FP_LONG TestManager::AccelClassic(float x, float acceleration, float exponent, float midpoint, bool gain) {
    return AccelClassic(FP64_FromFloat(x), FP64_FromFloat(acceleration), FP64_FromFloat(exponent),
                        FP64_FromFloat(midpoint), gain);
}

FP_LONG TestManager::AccelMotivity(float x, float acceleration, float exponent, float midpoint) {
    return AccelMotivity(FP64_FromFloat(x), FP64_FromFloat(acceleration), FP64_FromFloat(exponent),
                         FP64_FromFloat(midpoint));
}

FP_LONG TestManager::AccelSynchronous(float x, float sync_speed, float gamma, float smoothness, float motivity,
                                      bool gain) {
    return AccelSynchronous(FP64_FromFloat(x), FP64_FromFloat(sync_speed), FP64_FromFloat(gamma),
                            FP64_FromFloat(smoothness), FP64_FromFloat(motivity), gain);
}

FP_LONG TestManager::AccelJump(float x, float acceleration, float exponent, float midpoint, bool gain) {
    return AccelJump(FP64_FromFloat(x), FP64_FromFloat(acceleration), FP64_FromFloat(exponent),
                     FP64_FromFloat(midpoint), gain);
}

FP_LONG TestManager::AccelLUT(float x, float values_x[], float values_y[], unsigned long count) {
    auto *values_x_fp = new FP_LONG[count];
    auto *values_y_fp = new FP_LONG[count];
    for (unsigned long i = 0; i < count; i++) {
        values_x_fp[i] = FP64_FromFloat(values_x[i]);
        values_y_fp[i] = FP64_FromFloat(values_y[i]);
    }
    FP_LONG result = AccelLUT(FP64_FromFloat(x), values_x_fp, values_y_fp, count);
    delete[] values_x_fp;
    delete[] values_y_fp;
    return result;
}

FP_LONG TestManager::AccelLUT(float x) {
    return AccelLUT(FP64_FromFloat(x));
}

FP_LONG TestManager::AccelLinear(float x) {
    return ApplyGlobalPostParameters(accel_linear(&modesConst, &accelParams, ApplyGlobalPreParameters(FP64_FromFloat(x))));
}

FP_LONG TestManager::AccelPower(float x) {
    return ApplyGlobalPostParameters(accel_power(&modesConst, &accelParams, ApplyGlobalPreParameters(FP64_FromFloat(x))));
}

FP_LONG TestManager::AccelClassic(float x) {
    return ApplyGlobalPostParameters(accel_classic(&modesConst, &accelParams, ApplyGlobalPreParameters(FP64_FromFloat(x))));
}

FP_LONG TestManager::AccelMotivity(float x) {
    return ApplyGlobalPostParameters(accel_motivity(&modesConst, &accelParams, ApplyGlobalPreParameters(FP64_FromFloat(x))));
}

FP_LONG TestManager::AccelSynchronous(float x) {
    return ApplyGlobalPostParameters(accel_synchronous(&modesConst, &accelParams, ApplyGlobalPreParameters(FP64_FromFloat(x))));
}

FP_LONG TestManager::AccelNatural(float x) {
    return ApplyGlobalPostParameters(accel_natural(&modesConst, &accelParams, ApplyGlobalPreParameters(FP64_FromFloat(x))));
}

FP_LONG TestManager::AccelJump(float x) {
    return ApplyGlobalPostParameters(accel_jump(&modesConst, &accelParams, ApplyGlobalPreParameters(FP64_FromFloat(x))));
}

ModesConstants &TestManager::GetModesConstants() {
    return modesConst;
}

void TestManager::UpdateModesConstants() {
    update_constants(&accelParams, &modesConst);
    function.PreCacheConstants();
}

bool TestManager::ValidateConstants() {
    if (accelParams.acceleration_mode == AccelMode_Current)
        return false;

    // switch (g_AccelerationMode) {
    //     case AccelMode_Linear:
    //         break;
    //     case AccelMode_Power:
    //         break;
    //     case AccelMode_Classic:
    //         break;
    //     case AccelMode_Motivity:
    //         break;
    //     case AccelMode_Jump:
    //         break;
    //     case AccelMode_Lut: case AccelMode_CustomCurve:
    //         break;
    // }

    return true;
}

bool TestManager::ValidateFunctionGUI() {
    return function.ValidateSettings();
}

void TestManager::SetAccelMode(AccelMode mode) {
    accelParams.acceleration_mode = mode;
    function.params->accelMode = mode;
}

void TestManager::SetAcceleration(FP_LONG acceleration) {
    accelParams.acceleration = acceleration;
    function.params->accel = FP64_ToFloat(acceleration);
}

void TestManager::SetExponent(FP_LONG exponent) {
    accelParams.exponent = exponent;
    function.params->exponent = FP64_ToFloat(exponent);
}

void TestManager::SetMidpoint(FP_LONG midpoint) {
    accelParams.midpoint = midpoint;
    function.params->midpoint = FP64_ToFloat(midpoint);
}

void TestManager::SetMotivity(FP_LONG motivity) {
    accelParams.motivity = motivity;
    function.params->motivity = FP64_ToFloat(motivity);
}

void TestManager::SetSensitivity(FP_LONG sensitivity) {
    accelParams.sensitivity = sensitivity;
    function.params->sens = FP64_ToFloat(sensitivity);
}

void TestManager::SetSensitivityY(FP_LONG sensitivityY) {
    accelParams.ratio_yx = sensitivityY;
    function.params->ratioYX = FP64_ToFloat(sensitivityY);
}

void TestManager::SetOutCap(FP_LONG outCap) {
    accelParams.output_cap = outCap;
    function.params->outCap = FP64_ToFloat(outCap);
}

void TestManager::SetInCap(FP_LONG inCap) {
    accelParams.input_cap = inCap;
    function.params->inCap = FP64_ToFloat(inCap);
}

void TestManager::SetOffset(FP_LONG offset) {
    accelParams.offset = offset;
    function.params->offset = FP64_ToFloat(offset);
}

void TestManager::SetPreScale(FP_LONG preScale) {
    accelParams.prescale = preScale;
    function.params->preScale = FP64_ToFloat(preScale);
}

void TestManager::SetRotationAngle(FP_LONG rotationAngle) {
    accelParams.rotation_angle = rotationAngle;
    function.params->rotation = FP64_ToFloat(rotationAngle);
}

void TestManager::SetAngleSnap_Angle(FP_LONG angleSnap_Angle) {
    accelParams.angle_snap_angle = angleSnap_Angle;
    function.params->asAngle = FP64_ToFloat(angleSnap_Angle);
}

void TestManager::SetAngleSnap_Threshold(FP_LONG angleSnap_Threshold) {
    accelParams.angle_snap_threshold = angleSnap_Threshold;
    function.params->asThreshold = FP64_ToFloat(angleSnap_Threshold);
}

void TestManager::SetUseSmoothing(bool useSmoothing) {
    accelParams.use_smoothing = useSmoothing;
    function.params->useSmoothing = useSmoothing;
}

void TestManager::SetLutSize(unsigned long lutSize) {
    accelParams.lut_pairs = lutSize;
    function.params->lutSize = lutSize;
}

void TestManager::SetLutData_x(FP_LONG values[], unsigned long count) {
    SetLutSize(count);

    for (unsigned long i = 0; i < count; i++) {
        accelParams.lut_data_x[i] = values[i];
        function.params->lutDataX[i] = FP64_ToFloat(values[i]);
    }
}

void TestManager::SetLutData_y(FP_LONG values[], unsigned long count) {
    SetLutSize(count);

    for (unsigned long i = 0; i < count; i++) {
        accelParams.lut_data_y[i] = values[i];
        function.params->lutDataY[i] = FP64_ToFloat(values[i]);
    }
}

void TestManager::SetLutData(FP_LONG values_x[], FP_LONG values_y[], unsigned long count) {
    SetLutSize(count);
    SetLutData_x(values_x, count);
    SetLutData_y(values_y, count);
}

void TestManager::SetAcceleration(float acceleration) {
    SetAcceleration(FP64_FromFloat(acceleration));
}

void TestManager::SetExponent(float exponent) {
    SetExponent(FP64_FromFloat(exponent));
}

void TestManager::SetMidpoint(float midpoint) {
    SetMidpoint(FP64_FromFloat(midpoint));
}

void TestManager::SetMotivity(float motivity) {
    SetMotivity(FP64_FromFloat(motivity));
}

void TestManager::SetSensitivity(float sensitivity) {
    SetSensitivity(FP64_FromFloat(sensitivity));
}

void TestManager::SetSensitivityY(float sensitivityY) {
    SetSensitivityY(FP64_FromFloat(sensitivityY));
}

void TestManager::SetOutCap(float outCap) {
    SetOutCap(FP64_FromFloat(outCap));
}

void TestManager::SetInCap(float inCap) {
    SetInCap(FP64_FromFloat(inCap));
}

void TestManager::SetOffset(float offset) {
    SetOffset(FP64_FromFloat(offset));
}

void TestManager::SetPreScale(float preScale) {
    SetPreScale(FP64_FromFloat(preScale));
}

void TestManager::SetRotationAngle(float rotationAngle) {
    SetRotationAngle(FP64_FromFloat(rotationAngle));
}

void TestManager::SetAngleSnap_Angle(float angleSnap_Angle) {
    SetAngleSnap_Angle(FP64_FromFloat(angleSnap_Angle));
}

void TestManager::SetAngleSnap_Threshold(float angleSnap_Threshold) {
    SetAngleSnap_Threshold(FP64_FromFloat(angleSnap_Threshold));
}

void TestManager::SetLutData(float values_x[], float values_y[], unsigned long count) {
    auto *values_x_fp = new FP_LONG[count];
    auto *values_y_fp = new FP_LONG[count];
    for (unsigned long i = 0; i < count; i++) {
        values_x_fp[i] = FP64_FromFloat(values_x[i]);
        values_y_fp[i] = FP64_FromFloat(values_y[i]);
    }
    SetLutData(values_x_fp, values_y_fp, count);
    delete[] values_x_fp;
    delete[] values_y_fp;
}

float TestManager::EvalFloatFunc(float x) {
    function.params->accelMode = static_cast<AccelMode>(accelParams.acceleration_mode);
    return function.EvalFuncAt(x);
}
