#include <iostream>
#include <fstream>
#include <optional>
#include <vector>

// GUI helpers
#include "../../gui/ConfigHelper.h"
#include "../../gui/DriverHelper.h"

/// Every device the driver is attached to, or just the named one
static std::vector<Device> SelectDevices(const char *name) {
    auto devices = DriverHelper::DiscoverDevices();

    if (!name)
        return devices;

    for (const auto &device: devices)
        if (device.sysfs_name == name || device.name == name)
            return {device};

    std::cerr << "No such device: " << name << std::endl;
    return {};
}

static int ApplyConfig(const std::string &file, const char *device_name) {
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

    const auto devices = SelectDevices(device_name);
    if (devices.empty()) {
        std::cerr << "No device to apply the configuration to." << std::endl;
        return 1;
    }

    // Without a device given, the config is the same for every mouse
    int failed = 0;
    for (const auto &device: devices) {
        if (!params.SaveAll(device.params_dir)) {
            std::cerr << "Failed to apply the configuration to " << device.name << std::endl;
            failed++;
        }
    }

    if (failed == (int) devices.size())
        return 1;

    std::cout << "Configuration applied to " << devices.size() - failed << " device(s)." << std::endl;

    return 0;
}

static std::string DumpDriver(const char *device_name) {
    Parameters params{};

    char LUT_user_data[MAX_LUT_BUF_LEN];

    const auto devices = SelectDevices(device_name);
    if (devices.empty()) {
        std::cerr << "No device to read the configuration from." << std::endl;
        return {};
    }

    if (!DriverHelper::ParseAllParameters(devices.front().params_dir, params, LUT_user_data))
        return {};

    return ConfigHelper::ExportPlainText(params, false);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout <<
                "Usage:\n"
                "  yeetmousectl apply <config> [device]\n"
                "  yeetmousectl dump [device]\n"
                "  yeetmousectl save <file> [device]\n"
                "\n"
                "Devices are named as under /sys/class/yeetmouse. Without one, `apply` covers\n"
                "every mouse the driver is attached to and `dump`/`save` read the first one.\n";

        return 0;
    }

    const std::string cmd = argv[1];

    if (cmd == "apply") {
        if (argc < 3) {
            std::cerr << "Missing config file\n";
            return 2;
        }

        return ApplyConfig(argv[2], argc > 3 ? argv[3] : nullptr);
    }

    if (cmd == "dump") {
        const auto dump_str = DumpDriver(argc > 2 ? argv[2] : nullptr);
        if (dump_str.length() < 2) {
            return 4;
        }
        std::cout << dump_str;
        return 0;
    }

    if (cmd == "save") {
        if (argc < 3) {
            std::cerr << "Missing output file\n";
            return 2;
        }

        const auto dump_str = DumpDriver(argc > 3 ? argv[3] : nullptr);
        if (dump_str.length() < 2) {
            return 4;
        }

        std::ofstream out(argv[2]);
        if (!out.is_open()) {
            std::cerr << "Failed to open file\n";
            return 3;
        }

        out << dump_str;

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
float ImBezierQuadraticCalc(const ImVec2 &p1, const ImVec2 &p2, const ImVec2 &p3, float t) {
    throw std::logic_error("NOT YET IMPLEMENTED!");
}
