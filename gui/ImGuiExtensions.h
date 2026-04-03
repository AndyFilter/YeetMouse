#pragma once

#ifndef YEETMOUSE_IMGUIEXTENSIONS_H
#define YEETMOUSE_IMGUIEXTENSIONS_H

#include <ImGui/imgui.h>

namespace ImGui {
    bool ModeSelectable(const char *label, bool is_selected = false, ImGuiSelectableFlags flags = 0,
                        const ImVec2 &size = ImVec2(0, 0));

    bool ParameterSlider(const char *label, float &value, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f);
}

#endif // YEETMOUSE_IMGUIEXTENSIONS_H
