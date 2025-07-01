#ifndef YEETMOUSE_DRIVERHELPER_H
#define YEETMOUSE_DRIVERHELPER_H

#include <cmath>
#include <string>
#include <filesystem>
#include <algorithm>

#include "CustomCurve.h"
#include "../shared_definitions.h"

#define MAX_LUT_ARRAY_SIZE 128  // THIS NEEDS TO BE THE SAME AS IN THE DRIVER CODE

#define DEG2RAD (M_PI / 180.0)

namespace DriverHelper {
    bool GetParameterF(const std::string& param_name, float& value);
    bool GetParameterI(const std::string& param_name, int& value);
    bool GetParameterB(const std::string& param_name, bool& value);
    bool GetParameterS(const std::string& param_name, std::string& value);

    bool WriteParameterF(const std::string& param_name, float value);
    bool WriteParameterI(const std::string& param_name, float value);

    bool SaveParameters();

    bool ValidateDirectory();

    /// Converts the ugly FP64 representation of user parameters to nice floating point values.\n\n
    bool CleanParameters(int& fixed_num);

    /// Returns the number of parsed values
    size_t ParseUserLutData(char* user_data, double* out_x, double* out_y, size_t out_size);

    /// Returns the number of parsed values
    size_t ParseDriverLutData(const char* user_data, double* out_x, double* out_y);

    std::string EncodeLutData(double *data_x, double *data_y, size_t size);

} // DriverHelper

inline std::string AccelMode2String(AccelMode mode) {
    switch (mode) {
        case AccelMode_Current:
            return "Current";
        case AccelMode_Linear:
            return "Linear";
        case AccelMode_Power:
            return "Power";
        case AccelMode_Classic:
            return "Classic";
        case AccelMode_Motivity:
            return "Motivity";
        case AccelMode_Natural:
            return "Natural";
        case AccelMode_Jump:
            return "Jump";
        case AccelMode_Lut:
            return "LUT";
        default:
            return "Unknown";
    }
}

inline std::string AccelMode2String_CAPS(AccelMode mode) {
    switch (mode) {
        case AccelMode_Current:
            return "CURRENT";
        case AccelMode_Linear:
            return "LINEAR";
        case AccelMode_Power:
            return "POWER";
        case AccelMode_Classic:
            return "CLASSIC";
        case AccelMode_Motivity:
            return "MOTIVITY";
        case AccelMode_Natural:
        return "NATURAL";
        case AccelMode_Jump:
            return "JUMP";
        case AccelMode_Lut:
            return "LUT";
        default:
            return "Unknown";
    }
}

inline AccelMode AccelMode_From_String(std::string mode_text) {
    // Bring text to lowercase
    std::transform(mode_text.begin(), mode_text.end(), mode_text.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    if(mode_text == "current")
        return AccelMode_Current;
    else if (mode_text == "linear")
        return AccelMode_Linear;
    else if (mode_text == "power")
        return AccelMode_Power;
    else if (mode_text == "classic")
        return AccelMode_Classic;
    else if (mode_text == "motivity")
	return AccelMode_Motivity;
    else if (mode_text == "natural")
	return AccelMode_Natural;
    else if (mode_text == "jump")
        return AccelMode_Jump;
    else
        return AccelMode_Current;
}

struct Parameters {
    float sens = 1.0f; // Sensitivity for X axis only if sens != sensY (anisotropy is on), otherwise sensitivity for both axes
    float sensY = 1.0f; // Unused when anisotropy is off (sens == sensY)
    float outCap = 0.f;
    float inCap = 0.f;
    float offset = 0.0f;
    float accel = 2.0f;
    float exponent = 0.4f;
    float midpoint = 5.0f;
    float preScale = 1.0f;
    //float scrollAccel = 1.0f;
    AccelMode accelMode = AccelMode_Current;
    bool useSmoothing = true; // true/false
    float rotation = 0; // Stored in degrees, converted to radians when writing out
    float as_threshold = 0; // Stored in degrees, converted to radians when writing out
    float as_angle = 0; // Stored in degrees, converted to radians when writing out

    /// The issue of performance with LUT is currently solved with a fixed stride, but another approach would be to
    /// store separately x and y values, sort both by x values, and do a binary search every time you want to find points.
    /// ----- The so called "another approach" has been implemented -----
    //float LUT_stride = 1.f; // No longer used
    double LUT_data_x[MAX_LUT_ARRAY_SIZE];
    double LUT_data_y[MAX_LUT_ARRAY_SIZE];
    int LUT_size = 0;

    CustomCurve customCurve{};

    Parameters() = default;

    bool use_anisotropy = false; // This parameter is not saved anywhere, it's just a helper.
    // Anisotropy is on if sensY != sens, off otherwise.

    //Parameters(float sens, float sensCap, float speedCap, float offset, float accel, float exponent, float midpoint,
    //           float scrollAccel, int accelMode);

    bool SaveAll();
};

#endif //YEETMOUSE_DRIVERHELPER_H
