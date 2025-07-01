#include "TestManager.h"

#include "shared_definitions.h"
#include "driver/accel_modes.h"

// "Private" values only visible to the accel_modes
FP_LONG g_Acceleration = 0, g_Exponent = 0, g_Midpoint = 0, g_RotationAngle = 0, g_AngleSnap_Angle = 0, g_AngleSnap_Threshold = 0, g_LutData_x[256], g_LutData_y[256];
char g_AccelerationMode = 0, g_UseSmoothing = 0;
unsigned long g_LutSize = 0;
ModesConstants modesConst;
static CachedFunction function;

// TestManager & TestManager::GetInstance() {
//     static TestManager instance;
//     return instance;
// }

void TestManager::Initialize() {
    function.params = new Parameters;
    function.params->sens = 1;
    function.params->sensY = 1;
    function.params->accelMode = static_cast<AccelMode>(g_AccelerationMode);
    function.params->preScale = 1;
    function.params->accel = FP64_ToFloat(g_Acceleration);
    function.params->exponent = FP64_ToFloat(g_Exponent);
    function.params->midpoint = FP64_ToFloat(g_Midpoint);
    function.params->offset = 0;
    function.params->useSmoothing = g_UseSmoothing;
    function.params->rotation = FP64_ToFloat(g_RotationAngle);
    function.params->as_angle = FP64_ToFloat(g_AngleSnap_Angle);
    function.params->as_threshold = FP64_ToFloat(g_AngleSnap_Threshold);
    function.params->inCap = 0;
    function.params->outCap = 0;
    function.PreCacheConstants();
}

FP_LONG TestManager::AccelLinear(FP_LONG x, FP_LONG acceleration) {
    SetAcceleration(acceleration);
    UpdateModesConstants();
    return accel_linear(x);
}

FP_LONG TestManager::AccelPower(FP_LONG x, FP_LONG acceleration, FP_LONG exponent, FP_LONG midpoint) {
    SetAcceleration(acceleration);
    SetExponent(exponent);
    SetMidpoint(midpoint);
    UpdateModesConstants();
    return accel_power(x);
}

FP_LONG TestManager::AccelClassic(FP_LONG x, FP_LONG acceleration, FP_LONG exponent) {
    SetAcceleration(acceleration);
    SetExponent(exponent);
    UpdateModesConstants();
    return accel_classic(x);
}

FP_LONG TestManager::AccelMotivity(FP_LONG x, FP_LONG acceleration, FP_LONG exponent, FP_LONG midpoint) {
    SetAcceleration(acceleration);
    SetExponent(exponent);
    SetMidpoint(midpoint);
    UpdateModesConstants();
    return accel_motivity(x);
}

FP_LONG TestManager::AccelJump(FP_LONG x, FP_LONG acceleration, FP_LONG exponent, FP_LONG midpoint, bool gain) {
    SetAcceleration(acceleration);
    SetExponent(exponent);
    SetMidpoint(midpoint);
    SetUseSmoothing(gain);
    UpdateModesConstants();
    return accel_jump(x);
}

FP_LONG TestManager::AccelLUT(FP_LONG x, FP_LONG values_x[], FP_LONG values_y[], unsigned long count) {
    SetLutSize(count);
    SetLutData_x(values_x, count);
    SetLutData_y(values_y, count);
    UpdateModesConstants();
    return accel_lut(x);
}

FP_LONG TestManager::AccelLUT(FP_LONG x) {
    return accel_lut(x);
}

FP_LONG TestManager::AccelLinear(float x, float acceleration) {
    return AccelLinear(FP64_FromFloat(x), FP64_FromFloat(acceleration));
}

FP_LONG TestManager::AccelPower(float x, float acceleration, float exponent, float midpoint) {
    return AccelPower(FP64_FromFloat(x), FP64_FromFloat(acceleration), FP64_FromFloat(exponent), FP64_FromFloat(midpoint));
}

FP_LONG TestManager::AccelClassic(float x, float acceleration, float exponent) {
    return AccelClassic(FP64_FromFloat(x), FP64_FromFloat(acceleration), FP64_FromFloat(exponent));
}

FP_LONG TestManager::AccelMotivity(float x, float acceleration, float exponent, float midpoint) {
    return AccelMotivity(FP64_FromFloat(x), FP64_FromFloat(acceleration), FP64_FromFloat(exponent), FP64_FromFloat(midpoint));
}

FP_LONG TestManager::AccelJump(float x, float acceleration, float exponent, float midpoint, bool gain) {
    return AccelJump(FP64_FromFloat(x), FP64_FromFloat(acceleration), FP64_FromFloat(exponent), FP64_FromFloat(midpoint), gain);
}

FP_LONG TestManager::AccelLUT(float x, float values_x[], float values_y[], unsigned long count) {
    auto* values_x_fp = new FP_LONG[count];
    auto* values_y_fp = new FP_LONG[count];
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
    return accel_linear(FP64_FromFloat(x));
}

FP_LONG TestManager::AccelPower(float x) {
    return accel_power(FP64_FromFloat(x));
}

FP_LONG TestManager::AccelClassic(float x) {
    return accel_classic(FP64_FromFloat(x));
}

FP_LONG TestManager::AccelMotivity(float x) {
    return accel_motivity(FP64_FromFloat(x));
}

FP_LONG TestManager::AccelNatural(float x) {
    return accel_natural(FP64_FromFloat(x));
}

FP_LONG TestManager::AccelJump(float x) {
    return accel_jump(FP64_FromFloat(x));
}

ModesConstants& TestManager::GetModesConstants() {
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
    auto* values_x_fp = new FP_LONG[count];
    auto* values_y_fp = new FP_LONG[count];
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

// float TestManager::EvalFloatLinear(float x) {
//     function.params->accelMode = static_cast<AccelMode>(g_AccelerationMode);
//     return function.EvalFuncAt(x);
// }
//
// float TestManager::EvalFloatPower(float x) {
//     function.params->accelMode = static_cast<AccelMode>(g_AccelerationMode);
//     return function.EvalFuncAt(x);
// }
//
// float TestManager::EvalFloatClassic(float x) {
//     function.params->accelMode = static_cast<AccelMode>(g_AccelerationMode);
//     return function.EvalFuncAt(x);
// }
//
// float TestManager::EvalFloatMotivity(float x) {
//     function.params->accelMode = static_cast<AccelMode>(g_AccelerationMode);
//     return function.EvalFuncAt(x);
// }
//
// float TestManager::EvalFloatJump(float x) {
//     function.params->accelMode = static_cast<AccelMode>(g_AccelerationMode);
//     return function.EvalFuncAt(x);
// }
//
// float TestManager::EvalFloatLUT(float x) {
//     function.params->accelMode = static_cast<AccelMode>(g_AccelerationMode);
//     return function.EvalFuncAt(x);
// }
