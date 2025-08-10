#include "DriverHelper.h"
#include "External/FixedMath/Fixed64.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>
#include <set>

#include "External/ImGui/imgui_internal.h"
#include "External/ImGui/implot.h"

#define YEETMOUSE_PARAMS_DIR "/sys/module/yeetmouse/parameters/"

template<typename Ty>
bool GetParameterTy(const std::string& param_name, Ty &value) {
    try {
        using namespace std;
        ifstream file(YEETMOUSE_PARAMS_DIR + param_name);

        if(file.bad())
            return false;

        file >> value;
        file.close();
        return true;
    }
    catch (std::exception& ex) {
        fprintf(stderr, "Error when reading parameter %s (%s)\n", param_name.c_str(), ex.what());
        return false;
    }
}

bool GetParameterTy(const std::string& param_name, std::string &value) {
    try {
        using namespace std;
        ifstream file(YEETMOUSE_PARAMS_DIR + param_name);

        if(file.bad())
            return false;

        std::stringstream ss;
        ss << file.rdbuf();
        value = ss.str();
        file.close();
        return true;
    }
    catch (std::exception& ex) {
        fprintf(stderr, "Error when reading parameter %s (%s)\n", param_name.c_str(), ex.what());
        return false;
    }
}

template<typename Ty>
bool SetParameterTy(const std::string& param_name, Ty value) {
    try {
        using namespace std;
        ofstream file(YEETMOUSE_PARAMS_DIR + param_name);

        if (file.bad())
            return false;

        file << value;
        file.close();
        return true;
    }
    catch (std::exception& ex) {
        fprintf(stderr, "Error when saving parameter %s (%s)\n", param_name.c_str(), ex.what());
        return false;
    }
}

namespace DriverHelper {

    bool GetParameterF(const std::string& param_name, float &value) {
        return GetParameterTy(param_name, value);
    }

    bool GetParameterI(const std::string &param_name, int &value) {
        return GetParameterTy(param_name, value);
    }

    bool GetParameterB(const std::string& param_name, bool &value) {
        int temp = 0;
        bool res = GetParameterTy(param_name, temp);
        value = temp == 1;
        return res;
    }

    bool GetParameterS(const std::string &param_name, std::string &value) {
        return GetParameterTy(param_name, value);
    }

    bool CleanParameters(int& fixed_num) {
        namespace fs = std::filesystem;

        for (const auto & entry : fs::directory_iterator(YEETMOUSE_PARAMS_DIR)) {
            std::string str;
            std::ifstream t(entry.path());
            if(!t.is_open() || t.bad() || t.fail())
                return false;
            std::stringstream buffer;
            buffer << t.rdbuf();
            str = buffer.str();
            //printf("param at %s = %s\n", entry.path().c_str(), str.c_str());

            //std::streampos size = t.tellg();
            //std::cout << "pos = " << size << std::endl;
            //t.clear();
            //t.seekp(0);
            // I assume this is enough to not leave behind some parts of the old values if the new ones are shorter
            t.close();

            try {
                // Integer written with FP64_Shift
                if (size_t bracket_pos = str.find('('), ll_pos = str.find("ll");
                    str.find("<< 32") != std::string::npos &&bracket_pos != std::string::npos && ll_pos != std::string::npos)
                {
                    fixed_num++;
                    std::ofstream o(entry.path());
                    if(!o.is_open() || o.bad() || o.fail())
                        return false;
                    std::string int_str = str.substr(bracket_pos + 1, ll_pos - bracket_pos - 1);
                    //printf("Clean param: %s\n", int_str.c_str());
                    o.write(int_str.c_str(), int_str.size());
                    o.close();
                }
                else if (ll_pos != std::string::npos) { // Floating point represented as a long long
                    fixed_num++;
                    size_t start_offset = bracket_pos == std::string::npos ? 0 : (bracket_pos + 1);
                    std::ofstream o(entry.path());
                    if(!o.is_open() || o.bad() || o.fail())
                        return false;
                    std::string int_str = str.substr(start_offset, ll_pos - start_offset);
                    FP_LONG fp_val = std::stoll(int_str);
                    char buf[24];
                    FP64_ToString(fp_val, buf, 6);
                    //printf("Clean param: %s, which is %s\n", int_str.c_str(), buf);
                    o.write(buf, strlen(buf));
                    o.close();
                }
                else { // Anything else is either 0 or not meant to be a floating point
                    //printf("Wrong format \\;\n");
                }
            }
            catch (const std::exception& ex) {
                fprintf(stderr, "Error parsing parameter %s!\n", entry.path().filename().c_str());
                return false;
            }

        }

        // Save the new (clean) parameters. Nothing should change, it just looks nicer.
        SaveParameters();

        return true;
    }

    bool SaveParameters() {
        return SetParameterTy("update", (int)1);
    }

    bool WriteParameterF(const std::string &param_name, float value) {
        return SetParameterTy(param_name, value);
    }

    bool WriteParameterI(const std::string &param_name, float value) {
        return SetParameterTy(param_name, value);
    }

    bool ValidateDirectory() {
        namespace fs = std::filesystem;
        try {
            auto dir = fs::directory_entry(YEETMOUSE_PARAMS_DIR);
            if(!dir.exists())
                return false;
        }
        catch (std::exception &ex){
            return false;
        }

        return true;
    }

    size_t ParseUserLutData(char *szUser_data, double* out_x, double* out_y, size_t out_size) {
        if(!szUser_data) {
            strcpy(szUser_data, "Bad data pointer\0");
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
                ((idx % 2 == 0) ? out_x : out_y)[idx++ / 2] = p;

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
                strcpy(szUser_data, "Not enough values or bad formatting\0");
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
                }
                out_x[i] = pairs[i].first;
                out_y[i] = pairs[i].second;
            }

            return idx / 2;
        }
        catch (std::exception& ex) {
            printf("Error parsing user LUT data: %s\n", ex.what());
            return 0;
        }
    }

    size_t ParseDriverLutData(const char *szUser_data, double* out_x, double* out_y) {
        std::stringstream ss(szUser_data);
        size_t idx = 0;

        double p = 0;
        while(idx < MAX_LUT_ARRAY_SIZE * 2 && ss >> p) {
            //printf("idx = %zu, p = %f\n", idx, p);
            (idx % 2 == 0 ? out_x : out_y)[idx++/2] = p;

            char nextC = ss.peek();
            if (nextC == ';')
                ss.ignore();

            //idx++;
        }

        // 1 element is not enough for a linear interpolation
        if(idx <= 2 || idx % 2 == 1) {
            return 0;
        }

        return idx / 2;
    }

    std::string EncodeLutData(double *data_x, double *data_y, size_t size) {
        std::string res;

        for(int i = 0; i < size * 2; i++) {
                res += std::to_string(i % 2 == 0 ? data_x[i/2] : data_y[i/2]) + ";";
        }

        return res;
    }
} // DriverHelper

//Parameters::Parameters(float sens, float sensCap, float speedCap, float offset, float accel, float exponent,
//                       float midpoint, float scrollAccel, int accelMode) : sens(sens), outCap(sensCap),
//                                                                           inCap(speedCap), offset(offset),
//                                                                           accel(accel), exponent(exponent),
//                                                                           midpoint(midpoint), scrollAccel(scrollAccel),
//                                                                           accelMode(accelMode) {}

bool Parameters::SaveAll() {
    bool res = true;

    // General
    res &= SetParameterTy("Sensitivity", sens);
    res &= SetParameterTy("SensitivityY", use_anisotropy ? sensY : sens);
    res &= SetParameterTy("OutputCap", outCap);
    res &= SetParameterTy("InputCap", inCap);
    res &= SetParameterTy("Offset", offset);
    res &= SetParameterTy("AccelerationMode", accelMode);
    res &= SetParameterTy("RotationAngle", rotation * DEG2RAD);
    res &= SetParameterTy("AngleSnap_Threshold", as_threshold * DEG2RAD);
    res &= SetParameterTy("AngleSnap_Angle", as_angle * DEG2RAD);

    // Specific
    res &= SetParameterTy("Acceleration", accel);
    res &= SetParameterTy("Exponent", exponent);
    res &= SetParameterTy("Midpoint", midpoint);
    res &= SetParameterTy("Motivity", motivity);
    res &= SetParameterTy("PreScale", preScale);
    res &= SetParameterTy("UseSmoothing", useSmoothing);

    // LUT
    auto encodedLutData = DriverHelper::EncodeLutData(LUT_data_x, LUT_data_y, LUT_size);
    if(!encodedLutData.empty()) {
        res &= SetParameterTy("LutSize", LUT_size);
        //res &= SetParameterTy("LutStride", LUT_stride);
        //printf("encoded: %s, size: %zu, stride: %i\n", encoded.c_str(), LUT_size, LUT_stride);
        res &= SetParameterTy("LutDataBuf", encodedLutData);
    }
    else if(accelMode == AccelMode_Lut)
        return false;

    if(res)
        res &= DriverHelper::SaveParameters();

    return res;
}
