#ifndef GUI_CONFIGHELPER_H
#define GUI_CONFIGHELPER_H

#include "DriverHelper.h"
#include <string>

namespace ConfigHelper {
    std::string ExportPlainText(Parameters params, bool save_to_file);

    std::string ExportConfig(Parameters params, bool save_to_file);

    bool ImportFile(char *lut_data, Parameters &params);

    bool ImportClipboard(char *lut_data, const char *clipboard, Parameters &params);

    void SetRepoRoot(const std::string &root_path);
} // ConfigHelper

#endif //GUI_CONFIGHELPER_H
