#include "ConfigHelper.h"
#include <fstream>
#include <optional>
#include <unistd.h>

static char *OpenFile() {
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

static char *SaveFile() {
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
            res_ss << "ratioYX=" << (params.useAnisotropy ? params.ratioYX : 1) << std::endl;
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
            res_ss << "as_threshold=" << params.asThreshold << std::endl;
            res_ss << "as_angle=" << params.asAngle << std::endl;
            res_ss << "LUT_size=" << params.lutSize << std::endl;
            res_ss << "LUT_data=" << DriverHelper::EncodeLutData(params.lutDataX, params.lutDataY, params.lutSize, true) << std::endl;
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
            res_ss << "#define RATIO_YX " << (params.useAnisotropy ? params.ratioYX : 1) << std::endl;
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
            res_ss << "#define ANGLE_SNAPPING_THRESHOLD " << (params.asThreshold * DEG2RAD) << std::endl;
            res_ss << "#define ANGLE_SNAPPING_ANGLE " << (params.asAngle * DEG2RAD) << std::endl;
            res_ss << "#define LUT_SIZE " << params.lutSize << std::endl;
            res_ss << "#define LUT_DATA " << DriverHelper::EncodeLutData(
                params.lutDataX, params.lutDataY, params.lutSize, false) << std::endl;
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
