#ifndef YEETMOUSE_DRIVERHELPER_H
#define YEETMOUSE_DRIVERHELPER_H

#include <cmath>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>

#include "CustomCurve.h"
#include "../shared_definitions.h"

// Every mouse the driver attached to gets its own directory here, holding an `accel_config` group
#define YEETMOUSE_CLASS_DIR "/sys/class/yeetmouse/"
#define YEETMOUSE_DEVICE_PARAMS_SUBDIR "accel_config"

#define MAX_LUT_ARRAY_SIZE 128  // THIS NEEDS TO BE THE SAME AS IN THE DRIVER CODE
#define MAX_LUT_BUF_LEN 4096
#define LUT_EXPORT_PRECISION 5 // Decimal points precision for exporting a LUT

#define DEG2RAD (M_PI / 180.0)

struct Parameters;

struct Device {
    std::string sysfs_name; // Directory name under YEETMOUSE_CLASS_DIR (device name with spaces replaced by '_')
    std::string name; // Name as reported by the input device, meant to be displayed to the user
    std::string params_dir; // Absolute path of the device's parameter directory (with a trailing '/')
    bool readable = false; // Whether the parameters can be read (requires the 'yeetmouse' group)
    bool writable = false; // Whether the parameters can be written to
};

namespace DriverHelper {
    /// Lists every mouse the driver is currently attached to, sorted by name.
    /// Returns an empty vector when the driver isn't loaded.
    std::vector<Device> DiscoverDevices();

    /// Every parameter is read from and written to `params_dir` of the device it belongs to.
    /// The driver applies a parameter as soon as it is written, no separate update is needed.
    bool GetParameterF(const std::string &params_dir, const std::string &param_name, float &value);
    bool GetParameterI(const std::string &params_dir, const std::string &param_name, int &value);
    bool GetParameterB(const std::string &params_dir, const std::string &param_name, bool &value);
    bool GetParameterS(const std::string &params_dir, const std::string &param_name, std::string &value);

    bool WriteParameterF(const std::string &params_dir, const std::string &param_name, float value);
    bool WriteParameterI(const std::string &params_dir, const std::string &param_name, float value);

    /// Stores the driver configuration of `device` in /etc/yeetmouse.conf, asks for root privileges
    bool SavePersistentParameters(const Device &device);

    bool ValidateDirectory();

    /// Returns the number of parsed values
    size_t ParseUserLutData(char *user_data, double *out_x, double *out_y, size_t out_size);

    /// Returns the number of parsed values
    size_t ParseDriverLutData(const char *user_data, double *out_x, double *out_y);

    /// Reads all parameters of a single device
    bool ParseAllParameters(const std::string &params_dir, Parameters& params, char *lutUserData);

    std::string EncodeLutData(const double *data_x, const double *data_y, size_t size, bool strict_format = true);
} // DriverHelper

inline std::string AccelMode2String(AccelMode mode) {
    static_assert(AccelMode_Count == 10);

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
        case AccelMode_Synchronous:
            return "Synchronous";
        case AccelMode_Natural:
            return "Natural";
        case AccelMode_Jump:
            return "Jump";
        case AccelMode_Lut:
            return "LUT";
        case AccelMode_CustomCurve:
            return "Custom Curve";
        default:
            return "Unknown";
    }
}

inline std::string AccelMode2EnumString(AccelMode mode) {
    static_assert(AccelMode_Count == 10);

    switch (mode) {
        case AccelMode_Current:
            return "AccelMode_Current";
        case AccelMode_Linear:
            return "AccelMode_Linear";
        case AccelMode_Power:
            return "AccelMode_Power";
        case AccelMode_Classic:
            return "AccelMode_Classic";
        case AccelMode_Motivity:
            return "AccelMode_Motivity";
        case AccelMode_Synchronous:
            return "AccelMode_Synchronous";
        case AccelMode_Natural:
            return "AccelMode_Natural";
        case AccelMode_Jump:
            return "AccelMode_Jump";
        case AccelMode_Lut:
            return "AccelMode_Lut";
        case AccelMode_CustomCurve:
            return "AccelMode_CustomCurve";
        default:
            return "AccelMode_Current";
    }
}

inline std::string AccelMode2String_CAPS(AccelMode mode) {
    static_assert(AccelMode_Count == 10);

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
        case AccelMode_Synchronous:
            return "SYNCHRONOUS";
        case AccelMode_Jump:
            return "JUMP";
        case AccelMode_Lut:
            return "LUT";
        case AccelMode_CustomCurve:
            return "CUSTOM CURVE";
        default:
            return "Unknown";
    }
}

inline AccelMode AccelMode_From_String(std::string mode_text) {
    static_assert(AccelMode_Count == 10);

    // Bring text to lowercase
    std::transform(mode_text.begin(), mode_text.end(), mode_text.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (mode_text == "current")
        return AccelMode_Current;
    if (mode_text == "linear")
        return AccelMode_Linear;
    if (mode_text == "power")
        return AccelMode_Power;
    if (mode_text == "classic")
        return AccelMode_Classic;
    if (mode_text == "motivity")
        return AccelMode_Motivity;
    if (mode_text == "synchronous")
        return AccelMode_Synchronous;
    if (mode_text == "natural")
        return AccelMode_Natural;
    if (mode_text == "jump")
        return AccelMode_Jump;
    if (mode_text == "lut")
        return AccelMode_Lut;
    if (mode_text == "custom curve")
        return AccelMode_CustomCurve;
    return AccelMode_Current;
}

inline AccelMode AccelMode_From_EnumString(const std::string &mode_text) {
    static_assert(AccelMode_Count == 10);

    if (mode_text == "AccelMode_Linear")
        return AccelMode_Linear;
    if (mode_text == "AccelMode_Power")
        return AccelMode_Power;
    if (mode_text == "AccelMode_Classic")
        return AccelMode_Classic;
    if (mode_text == "AccelMode_Motivity")
        return AccelMode_Motivity;
    if (mode_text == "AccelMode_Synchronous")
        return AccelMode_Synchronous;
    if (mode_text == "AccelMode_Natural")
        return AccelMode_Natural;
    if (mode_text == "AccelMode_Jump")
        return AccelMode_Jump;
    if (mode_text == "AccelMode_Lut")
        return AccelMode_Lut;
    if (mode_text == "AccelMode_CustomCurve")
        return AccelMode_CustomCurve;

    return AccelMode_Current;
}

struct Parameters {
    float sens = 1.0f;
    float ratioYX = 1.0f; // Unused when anisotropy is off
    float outCap = 0.f;
    float inCap = 0.f;
    float offset = 0.0f;
    float preScale = 1.0f;
    float accel = 2.0f;
    float exponent = 0.4f;
    float midpoint = 5.0f;
    float motivity = 1.5f;
    //float scrollAccel = 1.0f;
    AccelMode accelMode = AccelMode_Current;
    bool useSmoothing = true; // true/false
    float rotation = 0; // Stored in degrees, converted to radians when writing out
    float asThreshold = 0; // Stored in degrees, converted to radians when writing out
    float asAngle = 0; // Stored in degrees, converted to radians when writing out

    double lutDataX[MAX_LUT_ARRAY_SIZE]{};
    double lutDataY[MAX_LUT_ARRAY_SIZE]{};
    int lutSize = 0;

    CustomCurve customCurve;

    Parameters() = default;

    bool useAnisotropy = false; // This parameter is not saved anywhere, it's just a helper.
    // Anisotropy is on if ratioYX != 1, off otherwise.

    //Parameters(float sens, float sensCap, float speedCap, float offset, float accel, float exponent, float midpoint,
    //           float scrollAccel, int accelMode);

    /// Writes every parameter to a single device, applied by the driver as they are written
    bool SaveAll(const std::string &params_dir) const;
};

#endif //YEETMOUSE_DRIVERHELPER_H
