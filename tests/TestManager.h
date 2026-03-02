#ifndef TESTMANAGER_H
#define TESTMANAGER_H

#include "shared_definitions.h"
#include "driver/accel_modes.h"
#include "../gui/FunctionHelper.h"

///
/// Class for interfacing with the 'fake' driver
///
class TestManager {
public:
    //static TestManager& GetInstance();
    static void Initialize();

    static FP_LONG AccelLinear(FP_LONG x, FP_LONG acceleration, FP_LONG midpoint, bool gain);
    static FP_LONG AccelPower(FP_LONG x, FP_LONG acceleration, FP_LONG exponent, FP_LONG midpoint, FP_LONG motivity,
                              bool gain);
    static FP_LONG AccelClassic(FP_LONG x, FP_LONG acceleration, FP_LONG exponent, FP_LONG midpoint, bool gain);
    static FP_LONG AccelMotivity(FP_LONG x, FP_LONG acceleration, FP_LONG exponent, FP_LONG midpoint);
    static FP_LONG AccelSynchronous(FP_LONG x, FP_LONG sync_speed, FP_LONG gamma, FP_LONG smoothness, FP_LONG motivity,
                                    bool gain);
    static FP_LONG AccelJump(FP_LONG x, FP_LONG acceleration, FP_LONG exponent, FP_LONG midpoint, bool gain);
    static FP_LONG AccelLUT(FP_LONG x, FP_LONG values_x[], FP_LONG values_y[], unsigned long count);
    static FP_LONG AccelLUT(FP_LONG x);

    static FP_LONG AccelLinear(float x, float acceleration, float midpoint, bool gain);
    static FP_LONG AccelPower(float x, float acceleration, float exponent, float midpoint, float motivity, bool gain);
    static FP_LONG AccelClassic(float x, float acceleration, float exponent, float midpoint, bool gain);
    static FP_LONG AccelMotivity(float x, float acceleration, float exponent, float midpoint);
    static FP_LONG AccelSynchronous(float x, float sync_speed, float gamma, float smoothness, float motivity,
                                    bool gain);
    static FP_LONG AccelJump(float x, float acceleration, float exponent, float midpoint, bool gain);
    static FP_LONG AccelLUT(float x, float values_x[], float values_y[], unsigned long count);

    static FP_LONG AccelLinear(float x); // Parameter values set manually!
    static FP_LONG AccelPower(float x); // Parameter values set manually!
    static FP_LONG AccelClassic(float x); // Parameter values set manually!
    static FP_LONG AccelMotivity(float x); // Parameter values set manually!
    static FP_LONG AccelSynchronous(float x); // Parameter values set manually!
    static FP_LONG AccelNatural(float x); // Parameter values set manually!
    static FP_LONG AccelJump(float x); // Parameter values set manually!
    static FP_LONG AccelLUT(float x); // Parameter values set manually!

    static ModesConstants &GetModesConstants();
    static void UpdateModesConstants();
    static bool ValidateConstants();
    static bool ValidateFunctionGUI();

    static void SetAccelMode(AccelMode mode);
    static void SetUseSmoothing(char useSmoothing);
    static void SetAcceleration(FP_LONG acceleration);
    static void SetExponent(FP_LONG exponent);
    static void SetMidpoint(FP_LONG midpoint);
    static void SetMotivity(FP_LONG motivity);
    static void SetSensitivity(FP_LONG sensitivity);
    static void SetSensitivityY(FP_LONG sensitivityY);
    static void SetOutCap(FP_LONG outCap);
    static void SetInCap(FP_LONG inCap);
    static void SetOffset(FP_LONG offset);
    static void SetPreScale(FP_LONG preScale);
    static void SetRotationAngle(FP_LONG rotationAngle);
    static void SetAngleSnap_Angle(FP_LONG angleSnap_Angle);
    static void SetAngleSnap_Threshold(FP_LONG angleSnap_Threshold);
    static void SetUseSmoothing(bool useSmoothing);
    static void SetLutSize(unsigned long lutSize);
    static void SetLutData_x(FP_LONG values[], unsigned long count);
    static void SetLutData_y(FP_LONG values[], unsigned long count);
    static void SetLutData(FP_LONG values_x[], FP_LONG values_y[], unsigned long count);

    static void SetAcceleration(float acceleration);
    static void SetExponent(float exponent);
    static void SetMidpoint(float midpoint);
    static void SetMotivity(float motivity);
    static void SetSensitivity(float sensitivity);
    static void SetSensitivityY(float sensitivityY);
    static void SetOutCap(float outCap);
    static void SetInCap(float inCap);
    static void SetOffset(float offset);
    static void SetPreScale(float preScale);
    static void SetRotationAngle(float rotationAngle);
    static void SetAngleSnap_Angle(float angleSnap_Angle);
    static void SetAngleSnap_Threshold(float angleSnap_Threshold);
    static void SetLutData(float values_x[], float values_y[], unsigned long count);

    // static float EvalFloatLinear(float x);
    // static float EvalFloatPower(float x);
    // static float EvalFloatClassic(float x);
    // static float EvalFloatMotivity(float x);
    // static float EvalFloatJump(float x);
    // static float EvalFloatLUT(float x);

    static float EvalFloatFunc(float x);
};


#endif //TESTMANAGER_H
