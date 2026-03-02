#include "ConfigHelper.h"
#include <fstream>
#include <optional>
#include <unistd.h>

char *OpenFile() {
    char *filename = new char[512];
    char cwd[1024];
    char command[2048] = R"(zenity --file-selection --title="Select a config file" 2> /dev/null)";
    FILE *f = nullptr;
    if (getcwd(cwd, sizeof(cwd)) != nullptr)
        sprintf(command, R"(zenity --file-selection --title="Select a config file" --filename="%s/" 2> /dev/null)",
                cwd);

    f = popen(command, "r");
    auto res = fgets(filename, 512, f);
    if (!res) {
        delete[] filename;
        return nullptr;
    }
    res[strlen(res) - 1] = 0;

    pclose(f);

    return res;
}

char *SaveFile() {
    char *filename = new char[512];
    char cwd[1024];
    char command[2048] = R"(zenity --save --file-selection --title="Save Config" 2> /dev/null)";
    FILE *f = nullptr;
    if (getcwd(cwd, sizeof(cwd)) != nullptr)
        sprintf(command, R"(zenity --save --file-selection --title="Save Config" --filename="%s/" 2> /dev/null)", cwd);

    f = popen(command, "r");
    auto res = fgets(filename, 512, f);
    if (!res) {
        delete[] filename;
        return nullptr;
    }
    res[strlen(res) - 1] = 0;

    pclose(f);

    return res;
}

namespace ConfigHelper {
    std::string ExportPlainText(Parameters params, bool save_to_file) {
        std::stringstream res_ss;

        try {
            res_ss << "sens=" << params.sens << std::endl;
            res_ss << "sens_Y=" << (params.use_anisotropy ? params.sensY : 1) << std::endl;
            res_ss << "outCap=" << params.outCap << std::endl;
            res_ss << "inCap=" << params.inCap << std::endl;
            res_ss << "offset=" << params.offset << std::endl;
            res_ss << "accel=" << params.accel << std::endl;
            res_ss << "exponent=" << params.exponent << std::endl;
            res_ss << "midpoint=" << params.midpoint << std::endl;
            res_ss << "motivity=" << params.motivity << std::endl;
            res_ss << "preScale=" << params.preScale << std::endl;
            res_ss << "accelMode=" << AccelMode2EnumString(params.accelMode) << std::endl;
            res_ss << "useSmoothing=" << params.useSmoothing << std::endl;
            res_ss << "rotation=" << params.rotation << std::endl;
            res_ss << "as_threshold=" << params.as_threshold << std::endl;
            res_ss << "as_angle=" << params.as_angle << std::endl;
            res_ss << "LUT_size=" << params.LUT_size << std::endl;
            res_ss << "LUT_data=" << DriverHelper::EncodeLutData(params.LUT_data_x, params.LUT_data_y, params.LUT_size, true) << std::endl;
            res_ss << "CC_data_aggregate=" << params.customCurve.ExportCustomCurve();

            if (save_to_file) {
                auto out_path = SaveFile();
                if (!out_path)
                    return "";
                std::ofstream out_file(out_path);

                delete[] out_path;

                if (!out_file.good())
                    return "";

                out_file << res_ss.str();

                out_file.close();
            }
            return res_ss.str();
        } catch (std::exception &ex) {
            printf("Failed Export: %s\n", ex.what());
        }

        return "";
    }

    std::string ExportConfig(Parameters params, bool save_to_file) {
        try {
            std::stringstream res_ss;

            res_ss << "#define SENSITIVITY " << params.sens << std::endl;
            res_ss << "#define SENSITIVITY_Y " << (params.use_anisotropy ? params.sensY : 1) << std::endl;
            res_ss << "#define OUTPUT_CAP " << params.outCap << std::endl;
            res_ss << "#define INPUT_CAP " << params.inCap << std::endl;
            res_ss << "#define OFFSET " << params.offset << std::endl;
            res_ss << "#define ACCELERATION " << params.accel << std::endl;
            res_ss << "#define EXPONENT " << params.exponent << std::endl;
            res_ss << "#define MIDPOINT " << params.midpoint << std::endl;
            res_ss << "#define MOTIVITY " << params.motivity << std::endl;
            res_ss << "#define PRESCALE " << params.preScale << std::endl;
            res_ss << "#define ACCELERATION_MODE " << AccelMode2EnumString(params.accelMode) << std::endl;
            res_ss << "#define USE_SMOOTHING " << params.useSmoothing << std::endl;
            res_ss << "#define ROTATION_ANGLE " << (params.rotation * DEG2RAD) << std::endl;
            res_ss << "#define ANGLE_SNAPPING_THRESHOLD " << (params.as_threshold * DEG2RAD) << std::endl;
            res_ss << "#define ANGLE_SNAPPING_ANGLE " << (params.as_angle * DEG2RAD) << std::endl;
            res_ss << "#define LUT_SIZE " << params.LUT_size << std::endl;
            res_ss << "#define LUT_DATA " << DriverHelper::EncodeLutData(
                params.LUT_data_x, params.LUT_data_y, params.LUT_size, false) << std::endl;
            res_ss << "#define CC_DATA_AGGREGATE " << params.customCurve.ExportCustomCurve();

            if (save_to_file) {
                auto out_path = SaveFile();
                if (!out_path)
                    return "";
                std::ofstream out_file(out_path);

                delete[] out_path;

                if (!out_file.good())
                    return "";

                out_file << res_ss.str();

                out_file.close();
            }
            return res_ss.str();
        } catch (std::exception &ex) {
            printf("Failed Export: %s\n", ex.what());
        }

        return "";
    }

#define STRING_2_LOWERCASE(s) std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });

    template<typename StreamType>
    std::optional<Parameters> ImportAny(StreamType &stream, char *lut_data, bool &is_config_h,
                                        bool *is_old_config = nullptr) {
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
            else if (name == "sens_y" || name == "sensitivity_y") {
                params.sensY = val;
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
                params.as_threshold = val / (is_config_h ? DEG2RAD : 1);
            else if (name == "as_angle" || name == "angle_snapping_angle")
                params.as_angle = val / (is_config_h ? DEG2RAD : 1);
            else if (name == "lut_size")
                params.LUT_size = val;
            else if (name == "lut_data") {
                strcpy(lut_data, val_str.c_str());
                params.LUT_size = DriverHelper::ParseUserLutData(lut_data, params.LUT_data_x, params.LUT_data_y,
                                                                 params.LUT_size);
                //DriverHelper::ParseDriverLutData(lut_data, params.LUT_data_x, params.LUT_data_y);
            } else if (name == "cc_data_aggregate") {
                params.customCurve.ImportCustomCurve(val_str);
            } else
                unknown_params++;

            idx++;
        }

        params.use_anisotropy = params.sensY != 1;

        if ((idx < 14 && unknown_params > 3) || unknown_params == idx) {
            printf("Bad config format, missing parameters\n");
            return {};
        }

        return params;
    }

    bool ImportFile(char *lut_data, Parameters &params) {
        const char *filepath = OpenFile();

        if (filepath == nullptr)
            return false;

        bool is_config_h = false;
        auto file_name_len = strlen(filepath);
        try {
            is_config_h = filepath[file_name_len - 1] == 'h' && filepath[file_name_len - 2] == '.';

            std::fstream file(filepath);

            if (!file.good())
                return {};

            bool is_old_config = false;
            if (auto res = ImportAny(file, lut_data, is_config_h, &is_old_config))
                params = res.value();
            else {
                file.close();
                return false;
            }

            file.close();

            // Automatically re-export in the correct format
            if (is_old_config) {
                std::ofstream out_file(filepath);

                if (out_file.is_open()) {
                    if (is_config_h)
                        out_file << ExportConfig(params, false);
                    else
                        out_file << ExportPlainText(params, false);

                    out_file.close();
                }
            }

            delete[] filepath;
        } catch (std::exception &ex) {
            delete[] filepath;
            printf("Import error: %s\n", ex.what());
            return false;
        }

        return true;
    }

    bool ImportClipboard(char *lut_data, const char *clipboard, Parameters &params) {
        if (clipboard == nullptr)
            return false;

        bool is_config_h = false;
        try {
            std::stringstream sstream(clipboard);

            bool is_old_config = false;
            if (auto res = ImportAny(sstream, lut_data, is_config_h, &is_old_config))
                params = res.value();
            else
                return false;

            // Automatically re-export in the correct format
            if (is_old_config) {
                if (is_config_h)
                    ImGui::SetClipboardText(ExportConfig(params, false).c_str());
                else
                    ImGui::SetClipboardText(ExportPlainText(params, false).c_str());
            }
        } catch (std::exception &ex) {
            printf("Import error: %s\n", ex.what());
            return false;
        }

        return true;
    }
} // ConfigHelper
