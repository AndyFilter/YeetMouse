#include "DriverHelper.h"
#include <FixedMath/Fixed64.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <sstream>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <set>

#include <ImGui/imgui_internal.h>
#include <ImGui/implot.h>

template<typename Ty>
static bool GetParameterTy(const std::string &path, Ty &value) {
    try {
        using namespace std;
        ifstream file(path);

        if (!(file >> value)) {
            fprintf(stderr, "Error when reading parameter %s (%s)\n", path.c_str(), strerror(errno));
            return false;
        }

        return true;
    } catch (std::exception &ex) {
        fprintf(stderr, "Error when reading parameter %s (%s)\n", path.c_str(), ex.what());
        return false;
    }
}

static bool GetParameterTy(const std::string &path, std::string &value) {
    try {
        using namespace std;
        ifstream file(path);

        if (!file.is_open()) {
            fprintf(stderr, "Error when reading parameter %s (%s)\n", path.c_str(), strerror(errno));
            return false;
        }

        std::stringstream ss;
        ss << file.rdbuf();
        value = ss.str();
        return true;
    } catch (std::exception &ex) {
        fprintf(stderr, "Error when reading parameter %s (%s)\n", path.c_str(), ex.what());
        return false;
    }
}

template<typename Ty>
static bool SetParameterTy(const std::string &path, Ty value) {
    try {
        using namespace std;
        ofstream file(path);

        // The driver rejects malformed values, which only shows up when the stream is flushed
        file << value;
        file.close();

        if (file.fail()) {
            fprintf(stderr, "Error when saving parameter %s (%s)\n", path.c_str(), strerror(errno));
            return false;
        }

        return true;
    } catch (std::exception &ex) {
        fprintf(stderr, "Error when saving parameter %s (%s)\n", path.c_str(), ex.what());
        return false;
    }
}

namespace DriverHelper {
    std::vector<Device> DiscoverDevices() {
        namespace fs = std::filesystem;

        std::vector<Device> devices;
        std::error_code ec;

        // The class directory only exists while the driver is loaded
        fs::directory_iterator it(YEETMOUSE_CLASS_DIR, fs::directory_options::skip_permission_denied, ec);
        if (ec)
            return devices;

        for (const auto &entry: it) {
            // Every entry is a symlink to the input device, holding the parameter group
            auto params_dir = entry.path() / YEETMOUSE_DEVICE_PARAMS_SUBDIR;
            if (!fs::is_directory(params_dir, ec))
                continue;

            Device device;
            device.sysfs_name = entry.path().filename().string();
            device.params_dir = params_dir.string() + "/";

            // The driver had to mangle the name to use it as a directory, so read the original one back
            std::ifstream name_file(entry.path() / "device" / "name");
            if (!std::getline(name_file, device.name) || device.name.empty()) {
                device.name = device.sysfs_name;
                std::replace(device.name.begin(), device.name.end(), '_', ' ');
            }

            // Every parameter of a device shares the same permissions, so one of them is enough to test
            const std::string probe = device.params_dir + "acceleration_mode";
            device.readable = access(probe.c_str(), R_OK) == 0;
            device.writable = access(probe.c_str(), W_OK) == 0;

            devices.push_back(std::move(device));
        }

        // Keep the order stable, the directory iteration order isn't
        std::sort(devices.begin(), devices.end(),
                  [](const Device &a, const Device &b) { return a.name < b.name; });

        return devices;
    }

    bool GetParameterF(const std::string &params_dir, const std::string &param_name, float &value) {
        return GetParameterTy(params_dir + param_name, value);
    }

    bool GetParameterI(const std::string &params_dir, const std::string &param_name, int &value) {
        return GetParameterTy(params_dir + param_name, value);
    }

    bool GetParameterB(const std::string &params_dir, const std::string &param_name, bool &value) {
        int temp = 0;
        bool res = GetParameterTy(params_dir + param_name, temp);
        value = temp == 1;
        return res;
    }

    bool GetParameterS(const std::string &params_dir, const std::string &param_name, std::string &value) {
        return GetParameterTy(params_dir + param_name, value);
    }

    bool SavePersistentParameters(const Device &device) {
        // Device names come straight out of the USB descriptors, so quote them before handing
        // the name to a shell
        std::string quoted_name = "'";
        for (char c: device.sysfs_name)
            quoted_name += (c == '\'') ? "'\\''" : std::string(1, c);
        quoted_name += "'";

        const std::string cmd = "pkexec /usr/bin/yeetmousectl save /etc/yeetmouse.conf " + quoted_name;
        return std::system(cmd.c_str()) == 0;
    }

    bool ValidateDirectory() {
        namespace fs = std::filesystem;
        std::error_code ec;

        return fs::is_directory(YEETMOUSE_CLASS_DIR, ec);
    }

    size_t ParseUserLutData(char *szUser_data, double *out_x, double *out_y, size_t out_size) {
        if (!szUser_data) {
            fprintf(stderr, "Error: User LUT data is empty!\n");
            return 0;
        }

        std::stringstream ss(szUser_data);
        size_t idx = 0;

        // Skip 2 equal pairs (it would cause kernel to panic...)
        std::set<double> visited_x;
        bool was_last_x_dup = false;

        try {
            double p = 0;
            while (idx < out_size * 2 && ss >> p) {
                if (idx % 2 == 1 && was_last_x_dup && out_y[(idx - 2) / 2] == p) {
                    idx--;
                    was_last_x_dup = false;
                    int skipped = 0;
                    char nextC = ss.peek();
                    while (nextC == ',' || nextC == ';' || (skipped > 0 && isspace(nextC))) {
                        ss.ignore();
                        nextC = ss.peek();
                        skipped++;
                    }
                    continue;
                }

                was_last_x_dup = false;
                if (idx % 2 == 0) {
                    if (visited_x.find(p) != visited_x.end())
                        was_last_x_dup = true;
                    else
                        visited_x.insert(p);
                }
                ((idx % 2 == 0) ? out_x : out_y)[idx / 2] = p;
                idx++;

                //((idx % 2 == 0) ? out_x : out_y)[idx++ / 2] = p;

                int skipped = 0;
                char nextC = ss.peek();
                while (nextC == ',' || nextC == ';' || (skipped > 0 && isspace(nextC))) {
                    ss.ignore();
                    nextC = ss.peek();
                    skipped++;
                }
            }

            //for(int i = 0; i < idx/2; i++) {
            //    printf("%f, ", out_x[i]);
            //}
            //printf("\n");

            // 1 element is not enough for a linear interpolation
            if (idx <= 2 || idx % 2 == 1) {
                strcpy(szUser_data, "Not enough values or bad formatting");
                return 0;
            }

            // Make sure all the data was parsed, if not then return 0
            if (!ss.eof()) {
                sprintf(szUser_data, "Too many samples! (%zu max)", out_size);
                fprintf(stderr, "Too many samples! (%zu max)\n", out_size);
                return 0;
            }

            // Zip the X and Y values for sorting
            std::pair<double, double> pairs[MAX_LUT_ARRAY_SIZE];
            for (int i = 0; i < idx / 2; i++)
                pairs[i] = std::make_pair(out_x[i], out_y[i]);

            // Sort the values together (according to X). While preserving the ordering in case of equal X values
            std::sort(pairs, pairs + idx / 2,
                      [](std::pair<double, double> a, std::pair<double, double> b) { return a.first < b.first; });

            // Unzip
            for (int i = 0; i < idx / 2; i++) {
                if (i >= 1) {
                    if (pairs[i].first == pairs[i - 1].first && pairs[i].second == pairs[i - 1].second) {
                        continue;
                    }

                    if (pairs[i-1].first > pairs[i].first) {
                        printf("Error: X values are not sorted! (x[i-1]: %f, x[i]: %f)\n", pairs[i-1].first, pairs[i].first);
                    }
                }
                out_x[i] = pairs[i].first;
                out_y[i] = pairs[i].second;
            }

            return idx / 2;
        } catch (std::exception &ex) {
            printf("Error parsing user LUT data: %s\n", ex.what());
            return 0;
        }
    }

    size_t ParseDriverLutData(const char *szUser_data, double *out_x, double *out_y) {
        std::stringstream ss(szUser_data);
        size_t idx = 0;

        double p = 0;
        while (idx < MAX_LUT_ARRAY_SIZE * 2 && ss >> p) {
            //printf("idx = %zu, p = %f\n", idx, p);
            (idx % 2 == 0 ? out_x : out_y)[idx / 2] = p;
            idx++;

            char nextC = ss.peek();
            if (nextC == ';' || nextC == ',')
                ss.ignore();

            //idx++;
        }

        // 1 element is not enough for a linear interpolation
        if (idx <= 2 || idx % 2 == 1) {
            return 0;
        }

        return idx / 2;
    }

    bool ParseAllParameters(const std::string &params_dir, Parameters &params, char *lutUserData) {
        bool res = true;

        res &= GetParameterF(params_dir, "sensitivity", params.sens);
        res &= GetParameterF(params_dir, "ratio_yx", params.ratioYX);
        res &= GetParameterF(params_dir, "output_cap", params.outCap);
        res &= GetParameterF(params_dir, "input_cap", params.inCap);
        res &= GetParameterF(params_dir, "offset", params.offset);
        res &= GetParameterF(params_dir, "acceleration", params.accel);
        res &= GetParameterF(params_dir, "exponent", params.exponent);
        res &= GetParameterF(params_dir, "midpoint", params.midpoint);
        res &= GetParameterF(params_dir, "motivity", params.motivity);
        res &= GetParameterF(params_dir, "prescale", params.preScale);
        int accelMode{};
        res &= GetParameterI(params_dir, "acceleration_mode", accelMode);
        params.accelMode = static_cast<AccelMode>(accelMode);
        res &= GetParameterB(params_dir, "use_smoothing", params.useSmoothing);
        res &= GetParameterF(params_dir, "rotation_angle", params.rotation);
        params.rotation /= DEG2RAD;
        res &= GetParameterF(params_dir, "angle_snap_threshold", params.asThreshold);
        params.asThreshold /= DEG2RAD;
        res &= GetParameterF(params_dir, "angle_snap_angle", params.asAngle);
        params.asAngle /= DEG2RAD;
        std::string Lut_dataBuf;
        res &= GetParameterS(params_dir, "lut_data", Lut_dataBuf);
        Lut_dataBuf.copy(lutUserData, MAX_LUT_BUF_LEN-1, 0);
        // The driver only stores the pairs themselves, the count comes back out of the data
        params.lutSize = ParseDriverLutData(Lut_dataBuf.c_str(), params.lutDataX, params.lutDataY);

        // Load custom curve data
        Lut_dataBuf.clear();
        if (res &= GetParameterS(params_dir, "cc_data_aggregate", Lut_dataBuf)) {
            CustomCurve dummy_curve;
            if (!dummy_curve.ImportCustomCurve(Lut_dataBuf) && params.accelMode == AccelMode_CustomCurve) {
                fprintf(stderr, "Could not load custom curve data\n");
                params.accelMode = AccelMode_Lut;
            }

            if (!dummy_curve.points.empty()) {
                params.customCurve = dummy_curve;
            }
        }

        params.useAnisotropy = params.ratioYX != params.sens;

        return res;
    }

    std::string EncodeLutData(const double *data_x, const double *data_y, size_t size, bool strict_format) {
        std::stringstream res;
        res << std::setprecision(LUT_EXPORT_PRECISION);

        for (int i = 0; i < size * 2; i++) {
            res << (i % 2 == 0 ? data_x[i / 2] : data_y[i / 2]) << ((strict_format && i % 2 == 0) ? "," : ";");
        }

        return res.str();
    }
} // DriverHelper

//Parameters::Parameters(float sens, float sensCap, float speedCap, float offset, float accel, float exponent,
//                       float midpoint, float scrollAccel, int accelMode) : sens(sens), outCap(sensCap),
//                                                                           inCap(speedCap), offset(offset),
//                                                                           accel(accel), exponent(exponent),
//                                                                           midpoint(midpoint), scrollAccel(scrollAccel),
//                                                                           accelMode(accelMode) {}

bool Parameters::SaveAll(const std::string &params_dir) const {
    bool res = true;

    // LUT
    auto encodedLutData = DriverHelper::EncodeLutData(lutDataX, lutDataY, lutSize);
    if (!encodedLutData.empty() && encodedLutData.size() < MAX_LUT_BUF_LEN) {
        res &= SetParameterTy(params_dir + "lut_data", encodedLutData);
    } else if (accelMode == AccelMode_Lut || accelMode == AccelMode_CustomCurve)
        return false;

    // Custom Curve
    auto encodedCCData = customCurve.ExportCustomCurve();
    if (!encodedCCData.empty() && encodedCCData.size() < MAX_LUT_BUF_LEN) {
        res &= SetParameterTy(params_dir + "cc_data_aggregate", encodedCCData);
    }
    else if (accelMode == AccelMode_CustomCurve)
        return false;

    // General
    res &= SetParameterTy(params_dir + "sensitivity", sens);
    res &= SetParameterTy(params_dir + "ratio_yx", useAnisotropy ? ratioYX : 1);
    res &= SetParameterTy(params_dir + "output_cap", outCap);
    res &= SetParameterTy(params_dir + "input_cap", inCap);
    res &= SetParameterTy(params_dir + "offset", offset);
    res &= SetParameterTy(params_dir + "rotation_angle", rotation * DEG2RAD);
    res &= SetParameterTy(params_dir + "angle_snap_threshold", asThreshold * DEG2RAD);
    res &= SetParameterTy(params_dir + "angle_snap_angle", asAngle * DEG2RAD);

    // Specific
    res &= SetParameterTy(params_dir + "acceleration", accel);
    res &= SetParameterTy(params_dir + "exponent", exponent);
    res &= SetParameterTy(params_dir + "midpoint", midpoint);
    res &= SetParameterTy(params_dir + "motivity", motivity);
    res &= SetParameterTy(params_dir + "prescale", preScale);
    res &= SetParameterTy(params_dir + "use_smoothing", useSmoothing);

    // Written last, the driver falls back to the current mode if the curve it needs is missing
    res &= SetParameterTy(params_dir + "acceleration_mode", accelMode);

    return res;
}
