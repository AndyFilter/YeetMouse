#ifndef GUI_CONFIGHELPER_H
#define GUI_CONFIGHELPER_H

#include <optional>
#include <sstream>
#include <string>
#include "DriverHelper.h"

namespace ConfigHelper {
    std::string ExportPlainText(Parameters params, bool save_to_file);

    std::string ExportConfig(Parameters params, bool save_to_file);

    template<typename StreamType>
    std::optional<Parameters> ImportAny(StreamType &stream, char *lut_data, bool &is_config_h, bool *is_old_config = nullptr);

    bool ImportFile(char *lut_data, Parameters &params);

    bool ImportClipboard(char *lut_data, const char *clipboard, Parameters &params);
} // ConfigHelper

#define STRING_2_LOWERCASE(s) std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });

template<typename StreamType>
std::optional<Parameters> ConfigHelper::ImportAny(StreamType &stream, char *lut_data, bool &is_config_h,
                                    bool *is_old_config) {
    static_assert(std::is_base_of<std::istream, StreamType>::value, "StreamType must be derived from std::istream");

    Parameters params;

    int unknown_params = 0;
    std::string line;
    int idx = 0;
    while (getline(stream, line)) {
        if (idx == 0 && (line.find("#define") != std::string::npos || line.find("//") != std::string::npos))
            is_config_h = true;

        if (is_config_h && line[0] == '/' && line[1] == '/')
            continue;

        std::string name;
        std::string val_str;
        double val = 0;

        std::string part;
        std::stringstream ss(line);
        for (int part_idx = 0; ss >> part; part_idx++) {
            if (is_config_h) {
                if (part_idx == 0)
                    continue;
                else if (part_idx == 1) {
                    name = part;
                    STRING_2_LOWERCASE(name);
                } else if (part_idx == 2) {
                    val_str = part;
                    try {
                        val = std::stod(val_str);
                    } catch (std::invalid_argument &_) {
                        val = NAN;
                    }
                } else
                    continue;
            } else {
                name = part.substr(0, part.find('='));
                STRING_2_LOWERCASE(name);
                val_str = part.substr(part.find('=') + 1);
                //printf("val str = %s\n", val_str.c_str());
                if (!val_str.empty()) {
                    try {
                        val = std::stod(val_str);
                    } catch (std::invalid_argument &_) {
                        val = NAN;
                    }
                }
            }
        }

        if (name == "sens" || name == "sensitivity")
            params.sens = val;
        else if (name == "ratio_yx" || name == "ratioyx" || name == "sens_y" || name == "sensitivity_y") {
            params.ratioYX = val;
        } else if (name == "outcap" || name == "output_cap")
            params.outCap = val;
        else if (name == "incap" || name == "input_cap")
            params.inCap = val;
        else if (name == "offset" || name == "output_cap")
            params.offset = val;
        else if (name == "acceleration" || name == "accel")
            params.accel = val;
        else if (name == "exponent")
            params.exponent = val;
        else if (name == "midpoint")
            params.midpoint = val;
        else if (name == "motivity")
            params.motivity = val;
        else if (name == "prescale")
            params.preScale = val;
        else if (name == "accelmode" || name == "acceleration_mode") {
            if (!std::isnan(val)) {
                // val +2 below for backward compatibility
                if (is_old_config)
                    *is_old_config = true;
                params.accelMode = static_cast<AccelMode>(std::clamp(
                    (int) val + (val > 4 ? 2 : 0), 0, (int) AccelMode_Count - 1));
            } else {
                if (is_old_config)
                    *is_old_config = false;
                params.accelMode = AccelMode_From_EnumString(val_str);
            }
        } else if (name == "usesmoothing" || name == "use_smoothing")
            params.useSmoothing = val;
        else if (name == "rotation" || name == "rotation_angle")
            params.rotation = val / (is_config_h ? DEG2RAD : 1);
        else if (name == "as_threshold" || name == "angle_snapping_threshold")
            params.asThreshold = val / (is_config_h ? DEG2RAD : 1);
        else if (name == "as_angle" || name == "angle_snapping_angle")
            params.asAngle = val / (is_config_h ? DEG2RAD : 1);
        else if (name == "lut_size")
            params.lutSize = val;
        else if (name == "lut_data") {
            strcpy(lut_data, val_str.c_str());
            params.lutSize = DriverHelper::ParseUserLutData(lut_data, params.lutDataX, params.lutDataY,
                                                             params.lutSize);
            //DriverHelper::ParseDriverLutData(lut_data, params.LUT_data_x, params.LUT_data_y);
        } else if (name == "cc_data_aggregate") {
            params.customCurve.ImportCustomCurve(val_str);
        } else
            unknown_params++;

        idx++;
    }

    params.useAnisotropy = params.ratioYX != 1;

    if ((idx < 14 && unknown_params > 3) || unknown_params == idx) {
        printf("Bad config format, missing parameters\n");
        return {};
    }

    return params;
}

#endif //GUI_CONFIGHELPER_H
