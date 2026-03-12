#include <iostream>
#include <fstream>
#include <optional>

// GUI helpers
#include "../../gui/ConfigHelper.h"
#include "../../gui/DriverHelper.h"

static int ApplyConfig(const std::string &file) {
    std::ifstream stream(file);

    if (!stream.is_open()) {
        std::cerr << "Failed to open config: " << file << std::endl;
        return 1;
    }

    char lut_data[4096] = {0};
    bool is_config_h = false;

    auto parsed = ConfigHelper::ImportAny(stream, (char *) lut_data, is_config_h);

    if (!parsed) {
        std::cerr << "Failed to parse config." << std::endl;
        return 1;
    }

    Parameters params = *parsed;

    params.SaveAll();

    std::cout << "Configuration applied." << std::endl;

    return 0;
}

static std::string DumpDriver() {
    Parameters params{};

    char LUT_user_data[MAX_LUT_BUF_LEN];

    DriverHelper::ParseAllParameters(params, LUT_user_data);

    return ConfigHelper::ExportPlainText(params, false);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout <<
                "Usage:\n"
                "  yeetmousectl apply <config>\n"
                "  yeetmousectl dump\n"
                "  yeetmousectl save <file>\n";

        return 0;
    }

    const std::string cmd = argv[1];

    if (cmd == "apply") {
        if (argc < 3) {
            std::cerr << "Missing config file\n";
            return 2;
        }

        return ApplyConfig(argv[2]);
    }

    if (cmd == "dump") {
        if (const auto dump_str = DumpDriver(); dump_str.length() < 2) {
            return 4;
        }
        std::cout << DumpDriver();
        return 0;
    }

    if (cmd == "save") {
        if (argc < 3) {
            std::cerr << "Missing output file\n";
            return 2;
        }

        std::ofstream out(argv[2]);
        if (!out.is_open()) {
            std::cerr << "Failed to open file\n";
            return 3;
        }

        out << DumpDriver();;

        return 0;
    }

    std::cerr << "Unknown command\n";
    return 1;
}

// ImGui stub, ignore
namespace ImGui {
    void SetClipboardText(const char *) {
        throw std::logic_error("NOT YET IMPLEMENTED!");
    }
}
float ImBezierCubicCalc(ImVec2 const&, ImVec2 const&, ImVec2 const&, ImVec2 const&, float) {
    throw std::logic_error("NOT YET IMPLEMENTED!");
}
