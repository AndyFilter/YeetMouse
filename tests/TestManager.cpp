#include "TestManager.h"

#include "shared_definitions.h"
#include "driver/accel_modes.h"

// "Private" values only visible to the accel_modes
FP_LONG g_Sensitivity = FP64_1, g_SensitivityY = FP64_1, g_OutputCap = 0, g_InputCap = 0, g_Offset = 0, g_PreScale = FP64_1, g_Acceleration = 0, g_Exponent
                = 0, g_Midpoint = 0, g_Motivity = 0, g_RotationAngle = 0, g_AngleSnap_Angle = 0, g_AngleSnap_Threshold =
                0, g_LutData_x[256], g_LutData_y[256];
char g_AccelerationMode = 0, g_UseSmoothing = 0;
unsigned long g_LutSize = 0;
ModesConstants modesConst;
static CachedFunction function;

// Ignores speedY (for now?)
FP_LONG ApplyGlobalPostParameters(FP_LONG speed) {
    FP_LONG speed_Y = FP64_1;
    if (g_SensitivityY == FP64_1) {
        if(g_Sensitivity != FP64_1)
            speed = FP64_Mul(speed, g_Sensitivity);

        // Apply Output Limit
        if(g_OutputCap > 0)
            speed = FP64_Min(g_OutputCap, speed);
    } else {
        speed = FP64_Mul(speed, g_Sensitivity);
        speed_Y = FP64_Mul(speed, g_SensitivityY);

        // Apply Output Limit
        if(g_OutputCap > 0) {
            speed = FP64_Min(g_OutputCap, speed);
            speed_Y = FP64_Min(g_OutputCap, speed_Y);
        }
    }

    return speed;
}

FP_LONG ApplyGlobalPreParameters(FP_LONG speed) {
    return FP64_Mul(speed, g_PreScale);
}

// TestManager & TestManager::GetInstance() {
//     static TestManager instance;
//     return instance;
// }

void TestManager::Initialize() {
    function.params = new Parameters;
    function.params->sens = FP64_ToFloat(g_Sensitivity);
    function.params->sensY = FP64_ToFloat(g_SensitivityY);
    function.params->accelMode = static_cast<AccelMode>(g_AccelerationMode);
    function.params->preScale = FP64_ToFloat(g_PreScale);
    function.params->accel = FP64_ToFloat(g_Acceleration);
    function.params->exponent = FP64_ToFloat(g_Exponent);
    function.params->midpoint = FP64_ToFloat(g_Midpoint);
    function.params->offset = FP64_ToFloat(g_Offset);
    function.params->useSmoothing = g_UseSmoothing;
    function.params->rotation = FP64_ToFloat(g_RotationAngle);
    function.params->as_angle = FP64_ToFloat(g_AngleSnap_Angle);
    function.params->as_threshold = FP64_ToFloat(g_AngleSnap_Threshold);
    function.params->inCap = 0;
    function.params->outCap = 0;
    function.PreCacheConstants();
}

FP_LONG TestManager::AccelLinear(FP_LONG x, FP_LONG acceleration, FP_LONG midpoint, bool gain) {
    SetAcceleration(acceleration);
    SetUseSmoothing(gain);
    SetMidpoint(midpoint);
    UpdateModesConstants();
    return ApplyGlobalPostParameters(accel_linear(ApplyGlobalPreParameters(x)));
}

FP_LONG TestManager::AccelPower(FP_LONG x, FP_LONG acceleration, FP_LONG exponent, FP_LONG midpoint, FP_LONG motivity,
                                bool gain) {
    SetAcceleration(acceleration);
    SetExponent(exponent);
    SetMidpoint(midpoint);
    SetMotivity(motivity);
    SetUseSmoothing(gain);
    UpdateModesConstants();
    return ApplyGlobalPostParameters(accel_power(ApplyGlobalPreParameters(x)));
}

FP_LONG TestManager::AccelClassic(FP_LONG x, FP_LONG acceleration, FP_LONG exponent, FP_LONG midpoint, bool gain) {
    SetAcceleration(acceleration);
    SetExponent(exponent);
    SetMidpoint(midpoint);
    SetUseSmoothing(gain);
    UpdateModesConstants();
    return ApplyGlobalPostParameters(accel_classic(ApplyGlobalPreParameters(x)));
}

FP_LONG TestManager::AccelMotivity(FP_LONG x, FP_LONG acceleration, FP_LONG exponent, FP_LONG midpoint) {
    SetAcceleration(acceleration);
    SetExponent(exponent);
    SetMidpoint(midpoint);
    UpdateModesConstants();
    return ApplyGlobalPostParameters(accel_motivity(ApplyGlobalPreParameters(x)));
}

FP_LONG TestManager::AccelSynchronous(FP_LONG x, FP_LONG sync_speed, FP_LONG gamma, FP_LONG smoothness,
                                      FP_LONG motivity, bool gain) {
    SetAcceleration(sync_speed);
    SetExponent(gamma);
    SetMidpoint(smoothness);
    SetMotivity(motivity);
    SetUseSmoothing(gain);
    UpdateModesConstants();
    return ApplyGlobalPostParameters(accel_synchronous(ApplyGlobalPreParameters(x)));
}

FP_LONG TestManager::AccelJump(FP_LONG x, FP_LONG acceleration, FP_LONG exponent, FP_LONG midpoint, bool gain) {
    SetAcceleration(acceleration);
    SetExponent(exponent);
    SetMidpoint(midpoint);
    SetUseSmoothing(gain);
    UpdateModesConstants();
    return ApplyGlobalPostParameters(accel_jump(ApplyGlobalPreParameters(x)));
}

FP_LONG TestManager::AccelLUT(FP_LONG x, FP_LONG values_x[], FP_LONG values_y[], unsigned long count) {
    SetLutSize(count);
    SetLutData_x(values_x, count);
    SetLutData_y(values_y, count);
    UpdateModesConstants();
    return ApplyGlobalPostParameters(accel_lut(ApplyGlobalPreParameters(x)));
}

FP_LONG TestManager::AccelLUT(FP_LONG x) {
    return ApplyGlobalPostParameters(accel_lut(ApplyGlobalPreParameters(x)));
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
    return ApplyGlobalPostParameters(accel_linear(ApplyGlobalPreParameters(FP64_FromFloat(x))));
}

FP_LONG TestManager::AccelPower(float x) {
    return ApplyGlobalPostParameters(accel_power(ApplyGlobalPreParameters(FP64_FromFloat(x))));
}

FP_LONG TestManager::AccelClassic(float x) {
    return ApplyGlobalPostParameters(accel_classic(ApplyGlobalPreParameters(FP64_FromFloat(x))));
}

FP_LONG TestManager::AccelMotivity(float x) {
    return ApplyGlobalPostParameters(accel_motivity(ApplyGlobalPreParameters(FP64_FromFloat(x))));
}

FP_LONG TestManager::AccelSynchronous(float x) {
    return ApplyGlobalPostParameters(accel_synchronous(ApplyGlobalPreParameters(FP64_FromFloat(x))));
}

FP_LONG TestManager::AccelNatural(float x) {
    return ApplyGlobalPostParameters(accel_natural(ApplyGlobalPreParameters(FP64_FromFloat(x))));
}

FP_LONG TestManager::AccelJump(float x) {
    return ApplyGlobalPostParameters(accel_jump(ApplyGlobalPreParameters(FP64_FromFloat(x))));
}

ModesConstants &TestManager::GetModesConstants() {
    return modesConst;
}

void TestManager::UpdateModesConstants() {
    update_constants();
    function.PreCacheConstants();
}

bool TestManager::ValidateConstants() {
    if (g_AccelerationMode == AccelMode_Current)
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
    g_AccelerationMode = mode;
    function.params->accelMode = static_cast<AccelMode>(g_AccelerationMode);
}

void TestManager::SetUseSmoothing(char useSmoothing) {
    g_UseSmoothing = useSmoothing;
    function.params->useSmoothing = g_UseSmoothing;
}

void TestManager::SetAcceleration(FP_LONG acceleration) {
    g_Acceleration = acceleration;
    function.params->accel = FP64_ToFloat(g_Acceleration);
}

void TestManager::SetExponent(FP_LONG exponent) {
    g_Exponent = exponent;
    function.params->exponent = FP64_ToFloat(g_Exponent);
}

void TestManager::SetMidpoint(FP_LONG midpoint) {
    g_Midpoint = midpoint;
    function.params->midpoint = FP64_ToFloat(g_Midpoint);
}

void TestManager::SetMotivity(FP_LONG motivity) {
    g_Motivity = motivity;
    function.params->motivity = FP64_ToFloat(g_Motivity);
}

void TestManager::SetSensitivity(FP_LONG sensitivity) {
    g_Sensitivity = sensitivity;
    function.params->sens = FP64_ToFloat(sensitivity);
}

void TestManager::SetSensitivityY(FP_LONG sensitivityY) {
    g_SensitivityY = sensitivityY;
    function.params->sensY = FP64_ToFloat(sensitivityY);
}

void TestManager::SetOutCap(FP_LONG outCap) {
    g_OutputCap = outCap;
    function.params->outCap = FP64_ToFloat(outCap);
}

void TestManager::SetInCap(FP_LONG inCap) {
    g_InputCap = inCap;
    function.params->inCap = FP64_ToFloat(inCap);
}

void TestManager::SetOffset(FP_LONG offset) {
    g_Offset = offset;
    function.params->offset = FP64_ToFloat(offset);
}

void TestManager::SetPreScale(FP_LONG preScale) {
    g_PreScale = preScale;
    function.params->preScale = FP64_ToFloat(preScale);
}

void TestManager::SetRotationAngle(FP_LONG rotationAngle) {
    g_RotationAngle = rotationAngle;
    function.params->rotation = FP64_ToFloat(g_RotationAngle);
}

void TestManager::SetAngleSnap_Angle(FP_LONG angleSnap_Angle) {
    g_AngleSnap_Angle = angleSnap_Angle;
    function.params->as_angle = FP64_ToFloat(g_AngleSnap_Angle);
}

void TestManager::SetAngleSnap_Threshold(FP_LONG angleSnap_Threshold) {
    g_AngleSnap_Threshold = angleSnap_Threshold;
    function.params->as_threshold = FP64_ToFloat(g_AngleSnap_Threshold);
}

void TestManager::SetUseSmoothing(bool useSmoothing) {
    g_UseSmoothing = useSmoothing ? 1 : 0;
    function.params->useSmoothing = g_UseSmoothing;
}

void TestManager::SetLutSize(unsigned long lutSize) {
    g_LutSize = lutSize;
    function.params->LUT_size = g_LutSize;
}

void TestManager::SetLutData_x(FP_LONG values[], unsigned long count) {
    SetLutSize(count);

    for (unsigned long i = 0; i < count; i++) {
        g_LutData_x[i] = values[i];
        function.params->LUT_data_x[i] = FP64_ToFloat(values[i]);
    }
}

void TestManager::SetLutData_y(FP_LONG values[], unsigned long count) {
    SetLutSize(count);

    for (unsigned long i = 0; i < count; i++) {
        g_LutData_y[i] = values[i];
        function.params->LUT_data_y[i] = FP64_ToFloat(values[i]);
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
    function.params->accelMode = static_cast<AccelMode>(g_AccelerationMode);
    return function.EvalFuncAt(x);
}
